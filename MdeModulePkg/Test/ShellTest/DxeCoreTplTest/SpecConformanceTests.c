/** @file
  Spec Conformance Tests (Tests 1-7).

  These tests verify PI Specification and UEFI Specification mandated behavior
  for TPL and interrupt management.  They validate that the system correctly
  implements RaiseTpl/RestoreTpl semantics and CPU Arch Protocol interrupt
  operations regardless of whether the timer recursion fix is present.

  Test 01: Normal RaiseTpl(TPL_HIGH_LEVEL)/RestoreTpl round-trip.
           Verifies interrupts are disabled at HIGH and re-enabled on restore.

  Test 02: RaiseTpl to non-HIGH levels (TPL_NOTIFY).
           Verifies interrupts remain enabled at levels below HIGH.

  Test 03: Nested RaiseTpl(HIGH) when already at TPL_HIGH_LEVEL.
           Verifies idempotent behavior - returns HIGH, no state corruption.

  Test 04: Normal TPL nesting via event dispatch.
           Signals events at TPL_NOTIFY and TPL_CALLBACK while at HIGH,
           then restores.  Verifies the event dispatch loop fires callbacks
           with interrupts enabled.

  Test 05: DisableInterrupt then RaiseTpl(HIGH).
           With interrupts already off, verifies RestoreTpl re-enables
           interrupts (PI Spec: TPL system owns interrupt state below HIGH).

  Test 06: DisableInterrupt while at TPL_HIGH_LEVEL.
           Calling DisableInterrupt when already at HIGH is redundant but
           must not corrupt state.  Verifies RestoreTpl still re-enables
           interrupts correctly.

  Test 07: Toggle interrupts without any TPL change.
           DisableInterrupt/EnableInterrupt without RaiseTpl/RestoreTpl.
           Verifies the TPL system is not affected by interrupt toggling alone.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "DxeCoreTplTest.h"

// ============================================================================
// Test 1 - Normal RaiseTpl(HIGH) / RestoreTpl
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test01RaiseTplHighRestoreTpl (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_TPL  OldTpl;

  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  UT_ASSERT_FALSE (GetInterruptStateChecked ());

  gBS->RestoreTPL (OldTpl);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  return UNIT_TEST_PASSED;
}

// ============================================================================
// Test 2 - RaiseTpl to non-HIGH
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test02RaiseTplNonHigh (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_TPL  OldTpl;

  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  gBS->RestoreTPL (OldTpl);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  return UNIT_TEST_PASSED;
}

// ============================================================================
// Test 3 - Nested RaiseTpl(HIGH) when already at HIGH
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test03NestedRaiseTplAtHigh (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_TPL  OldTpl;
  EFI_TPL  InnerOldTpl;

  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  UT_ASSERT_FALSE (GetInterruptStateChecked ());

  InnerOldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  UT_ASSERT_EQUAL (InnerOldTpl, (EFI_TPL)TPL_HIGH_LEVEL);

  gBS->RestoreTPL (InnerOldTpl);
  UT_ASSERT_FALSE (GetInterruptStateChecked ());

  gBS->RestoreTPL (OldTpl);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  return UNIT_TEST_PASSED;
}

// ============================================================================
// Test 4 - Event dispatch with interrupts enabled
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test04EventDispatchInterrupts (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   NotifyEvent;
  EFI_EVENT   CallbackEvent;
  EFI_TPL     OldTpl;

  mNotifyCallbackFired    = FALSE;
  mNotifyInterruptState   = FALSE;
  mCallbackDispatchCount  = 0;
  mCallbackInterruptState = FALSE;

  //
  // Create events at TPL_NOTIFY and TPL_CALLBACK
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  NotifyRecordCallback,
                  NULL,
                  &NotifyEvent
                  );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  CallbackRecordCallback,
                  NULL,
                  &CallbackEvent
                  );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  UT_ASSERT_FALSE (GetInterruptStateChecked ());

  gBS->SignalEvent (NotifyEvent);
  gBS->SignalEvent (CallbackEvent);

  gBS->RestoreTPL (OldTpl);

  //
  // Events should have been dispatched with interrupts enabled
  //
  UT_ASSERT_TRUE (mNotifyCallbackFired);
  UT_ASSERT_TRUE (mNotifyInterruptState);
  UT_ASSERT_TRUE (mCallbackDispatchCount > 0);
  UT_ASSERT_TRUE (mCallbackInterruptState);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  gBS->CloseEvent (NotifyEvent);
  gBS->CloseEvent (CallbackEvent);

  return UNIT_TEST_PASSED;
}

// ============================================================================
// Test 5 - DisableInterrupt then RaiseTpl(HIGH)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test05DisableIrqThenRaiseTpl (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_TPL  OldTpl;

  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  gCpu->DisableInterrupt (gCpu);
  UT_ASSERT_FALSE (GetInterruptStateChecked ());

  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->RestoreTPL (OldTpl);

  //
  // RestoreTpl to below HIGH always re-enables interrupts.  The TPL system
  // manages interrupt state: below HIGH means interrupts are enabled,
  // regardless of what the caller did before RaiseTpl.
  //
  // Implementation note: DisableInterrupt() + RaiseTpl(HIGH) causes
  // CoreRaiseTpl to set a stale value in gTplBeforeHighTpl (because
  // GetInterruptState() reads IF=0).  CoreRestoreTpl clears
  // gTplBeforeHighTpl when transitioning from HIGH to below HIGH,
  // before entering the event dispatch loop.  This prevents the stale
  // value from propagating to gIsrEntryTplMask via CoreTimerTick.
  //
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  //
  // Verify a second RaiseTpl/RestoreTpl cycle also leaves interrupts enabled.
  //
  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->RestoreTPL (OldTpl);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  return UNIT_TEST_PASSED;
}

// ============================================================================
// Test 6 - DisableInterrupt at HIGH (no-op)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test06DisableIrqAtHigh (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_TPL  OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  UT_ASSERT_FALSE (GetInterruptStateChecked ());

  gCpu->DisableInterrupt (gCpu);
  UT_ASSERT_FALSE (GetInterruptStateChecked ());

  gBS->RestoreTPL (OldTpl);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  return UNIT_TEST_PASSED;
}

// ============================================================================
// Test 7 - Toggle interrupts without TPL change
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test07ToggleIrqNoTpl (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_TPL  OldTpl;

  gCpu->DisableInterrupt (gCpu);
  gCpu->EnableInterrupt (gCpu);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->RestoreTPL (OldTpl);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  return UNIT_TEST_PASSED;
}
