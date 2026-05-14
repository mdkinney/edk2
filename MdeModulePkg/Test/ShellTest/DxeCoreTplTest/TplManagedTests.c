/** @file
  TPL-Managed Interrupt Scenario Tests (Tests 1-8).

  These tests verify normal TPL operations work correctly with the
  mIsrEntryTplMask fix in place.  They exercise the standard
  RaiseTpl/RestoreTpl paths that UEFI drivers and applications use daily.

  Test 01: Normal RaiseTpl(TPL_HIGH_LEVEL)/RestoreTpl round-trip.
           Verifies interrupts are disabled at HIGH and re-enabled on restore.

  Test 02: RaiseTpl to non-HIGH levels (TPL_NOTIFY).
           Verifies interrupts remain enabled at levels below HIGH.

  Test 03: Nested RaiseTpl(HIGH) when already at TPL_HIGH_LEVEL.
           Verifies idempotent behavior - returns HIGH, no state corruption.

  Test 04: No recursion under sustained timer load (1000 iterations).
           Rapidly cycles RaiseTpl/RestoreTpl with Stall() between iterations
           to guarantee timer interrupts fire.  Watchdog detects hang.

  Test 05: Normal context restored after timer IRQ.
           After allowing timer interrupts to fire, verifies the next
           RaiseTpl/RestoreTpl cycle works normally (mask self-cleans).

  Test 06: Temporarily lower TPL across the HIGH boundary.
           Goes HIGH -> APPLICATION -> HIGH -> restore.  Verifies the mask
           is properly managed across non-monotonic TPL transitions.

  Test 07: Normal TPL nesting via event dispatch.
           Signals events at TPL_NOTIFY and TPL_CALLBACK while at HIGH,
           then restores.  Verifies the event dispatch loop fires callbacks
           with interrupts enabled.

  Test 08: Timer IRQ during normal event dispatch.
           Uses a slow callback (stalls 2+ timer periods) to guarantee a
           timer interrupt fires DURING event dispatch.  Verifies no crash
           and subsequent operations work normally.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "DxeCoreTplTest.h"

// ============================================================================
// Test 1 - Normal RaiseTpl(HIGH) / RestoreTpl (Scenario 1)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test01NormalRaiseTplRestoreTpl (
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
// Test 2 - RaiseTpl to non-HIGH (Scenario 2)
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
// Test 3 - Nested RaiseTpl(HIGH) when already at HIGH (Scenario 3)
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
// Test 4 - Timer interrupt does not cause recursion (Scenarios 4, 5)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test04NoRecursionUnderTimer (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_TPL  OldTpl;
  UINTN    Index;

  if (IsResumeAfterWatchdogReset (Context)) {
    UT_LOG_ERROR ("Test failed on previous boot (watchdog reset - infinite recursion detected)");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  SaveStateBeforeDangerousTest ();
  SetRecursionWatchdog (30);

  for (Index = 0; Index < 1000; Index++) {
    OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
    gBS->RestoreTPL (OldTpl);
    gBS->Stall (mTimerPeriodUs);
  }

  ClearRecursionWatchdog ();
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  return UNIT_TEST_PASSED;
}

// ============================================================================
// Test 5 - Normal context after timer interrupt (Scenario 6)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test05NormalContextAfterTimerIrq (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_TPL  OldTpl;

  //
  // Stall to ensure at least one timer interrupt fires
  //
  gBS->Stall (StallForTicks (2));

  //
  // Next RaiseTpl/RestoreTpl should work normally
  //
  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->RestoreTPL (OldTpl);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  return UNIT_TEST_PASSED;
}

// ============================================================================
// Test 6 - Temporarily lower TPL across HIGH boundary (Scenario 7)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test06TempLowerAcrossHigh (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_TPL  InnerOldTpl;

  //
  // Raise to HIGH (we intentionally restore directly to APPLICATION below)
  //
  gBS->RaiseTPL (TPL_HIGH_LEVEL);
  UT_ASSERT_FALSE (GetInterruptStateChecked ());

  gBS->RestoreTPL (TPL_APPLICATION);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  //
  // Timer interrupts fire in this window
  //
  gBS->Stall (StallForTicks (2));

  InnerOldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  UT_ASSERT_FALSE (GetInterruptStateChecked ());

  gBS->RestoreTPL (InnerOldTpl);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  return UNIT_TEST_PASSED;
}

// ============================================================================
// Test 7 - Normal TPL nesting through event dispatch loop (Scenario 9)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test07NormalTplNesting (
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
// Test 8 - Timer interrupt during normal event dispatch (Scenario 5)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test08TimerIrqDuringDispatch (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   SlowEvent;
  EFI_TPL     OldTpl;

  mCallbackSurvived       = FALSE;
  mCallbackInterruptState = FALSE;

  if (IsResumeAfterWatchdogReset (Context)) {
    UT_LOG_ERROR ("Test failed on previous boot (watchdog reset - infinite recursion detected)");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  SlowNotifyCallback,
                  NULL,
                  &SlowEvent
                  );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  SaveStateBeforeDangerousTest ();
  SetRecursionWatchdog (30);

  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->SignalEvent (SlowEvent);
  gBS->RestoreTPL (OldTpl);

  ClearRecursionWatchdog ();

  UT_ASSERT_TRUE (mCallbackSurvived);
  UT_ASSERT_TRUE (mCallbackInterruptState);

  //
  // Verify self-clean: next RaiseTpl/RestoreTpl works normally
  //
  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->RestoreTPL (OldTpl);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  gBS->CloseEvent (SlowEvent);

  return UNIT_TEST_PASSED;
}

// ============================================================================
// Test 26 - TPL preemption hierarchy: CALLBACK preempted by NOTIFY via timer
//
// Verifies bounded preemption works correctly:
//   - A slow CALLBACK event handler is dispatched from normal context.
//   - A periodic timer event at TPL_NOTIFY fires via timer interrupt DURING
//     the CALLBACK handler's stall.
//   - The timer ISR's RestoreTpl dispatches the NOTIFY event because
//     gEfiCurrentTpl (NOTIFY) > HighBitSet64(mIsrEntryTplMask) (CALLBACK).
//   - The NOTIFY event executes, proving preemption occurred.
//   - The system does not recurse unboundedly (max nesting = 3 TPL levels).
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test26TplPreemptionHierarchy (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   PeriodicNotifyEvent;
  EFI_EVENT   SlowCallbackEvent;
  EFI_TPL     OldTpl;

  if (IsResumeAfterWatchdogReset (Context)) {
    UT_LOG_ERROR ("Test failed on previous boot (watchdog reset - infinite recursion detected)");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  //
  // Reset preemption test state
  //
  mNotifyDispatchCount       = 0;
  mPreemptNotifyCountAtStart = 0;
  mPreemptNotifyCountAtEnd   = 0;
  mPreemptCallbackStarted   = FALSE;
  mPreemptCallbackFinished  = FALSE;

  //
  // Create a periodic timer event at TPL_NOTIFY.
  // CoreTimerTick will signal this every timer period, and the timer ISR's
  // RestoreTpl will dispatch it — preempting the CALLBACK handler if bounded
  // preemption is working.
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  PreemptNotifyCallback,
                  NULL,
                  &PeriodicNotifyEvent
                  );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = gBS->SetTimer (PeriodicNotifyEvent, TimerPeriodic, mTimerPeriodUs * 10);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Create a slow CALLBACK event whose handler stalls for multiple timer
  // periods, giving the timer interrupt time to fire and preempt.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  PreemptSlowCallbackHandler,
                  NULL,
                  &SlowCallbackEvent
                  );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  SaveStateBeforeDangerousTest ();
  SetRecursionWatchdog (30);

  //
  // Signal the CALLBACK event while at HIGH, then restore to APP.
  // RestoreTpl dispatches the CALLBACK handler with interrupts enabled.
  // During the CALLBACK stall, timer interrupt fires and dispatches NOTIFY.
  //
  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->SignalEvent (SlowCallbackEvent);
  gBS->RestoreTPL (OldTpl);

  ClearRecursionWatchdog ();

  //
  // Verify the CALLBACK handler ran to completion
  //
  UT_ASSERT_TRUE (mPreemptCallbackStarted);
  UT_ASSERT_TRUE (mPreemptCallbackFinished);

  //
  // Verify preemption: NOTIFY event was dispatched DURING the CALLBACK
  // handler's stall (count at end > count at start).
  //
  UT_LOG_INFO (
    "NOTIFY dispatch count: start=%u, end=%u\n",
    (UINT32)mPreemptNotifyCountAtStart,
    (UINT32)mPreemptNotifyCountAtEnd
    );
  UT_ASSERT_TRUE (mPreemptNotifyCountAtEnd > mPreemptNotifyCountAtStart);

  //
  // Verify the system is still healthy
  //
  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->RestoreTPL (OldTpl);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  //
  // Clean up
  //
  gBS->SetTimer (PeriodicNotifyEvent, TimerCancel, 0);
  gBS->CloseEvent (PeriodicNotifyEvent);
  gBS->CloseEvent (SlowCallbackEvent);

  return UNIT_TEST_PASSED;
}
