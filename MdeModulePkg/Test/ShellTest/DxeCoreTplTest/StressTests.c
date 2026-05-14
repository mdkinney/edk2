/** @file
  Stress and Stability Tests (Tests 20-23).

  These tests verify the mIsrEntryTplMask fix remains stable under
  sustained load and does not degrade over time.  They exercise high-frequency
  TPL operations with timer interrupts continuously firing, ensuring no
  resource leaks, mask corruption, or performance degradation.

  Test 20: Sustained RaiseTpl/RestoreTpl - 100,000 iterations.
           Tight loop of RaiseTpl(HIGH)/RestoreTpl with no stall between
           iterations.  Timer interrupts fire asynchronously throughout.
           Verifies no stack overflow, no mask accumulation, and interrupts
           remain functional after the loop.  Uses 60-second watchdog.

  Test 21: Rapid TPL cycling across all levels - 10,000 iterations.
           Each iteration raises through CALLBACK -> NOTIFY -> HIGH then
           restores back down.  Exercises multi-level nesting where the
           event dispatch loop may fire at each intermediate level.

  Test 22: Timer event signaling during TPL cycling.
           Creates a periodic timer event (100ms period) and verifies
           callbacks fire correctly while interleaving RaiseTpl/RestoreTpl
           cycles.  Confirms the fix does not suppress event dispatch.

  Test 23: Non-HIGH RestoreTpl after timer IRQ (regression test).
           After ensuring a timer interrupt fires (leaving the mask potentially
           non-zero), performs non-HIGH TPL cycles (CALLBACK, NOTIFY) which
           should NOT consult the mask.  Then verifies a HIGH cycle still
           works.  Catches regressions where non-HIGH paths accidentally
           interact with the interrupt mask.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "DxeCoreTplTest.h"

// ============================================================================
// Test 20 - Sustained RaiseTpl/RestoreTpl under timer load
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test20SustainedStress (
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
  SetRecursionWatchdog (60);

  for (Index = 0; Index < 100000; Index++) {
    OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
    gBS->RestoreTPL (OldTpl);
  }

  ClearRecursionWatchdog ();
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  return UNIT_TEST_PASSED;
}

// ============================================================================
// Test 21 - Rapid TPL cycling across all levels
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test21RapidTplCycling (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_TPL  Tpl1;
  EFI_TPL  Tpl2;
  UINTN    Index;

  if (IsResumeAfterWatchdogReset (Context)) {
    UT_LOG_ERROR ("Test failed on previous boot (watchdog reset - infinite recursion detected)");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  SaveStateBeforeDangerousTest ();
  SetRecursionWatchdog (30);

  for (Index = 0; Index < 10000; Index++) {
    Tpl1 = gBS->RaiseTPL (TPL_CALLBACK);
    Tpl2 = gBS->RaiseTPL (TPL_NOTIFY);
    (VOID)gBS->RaiseTPL (TPL_HIGH_LEVEL);
    gBS->RestoreTPL (Tpl2);
    gBS->RestoreTPL (Tpl1);
    gBS->RestoreTPL (TPL_APPLICATION);
  }

  ClearRecursionWatchdog ();
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  return UNIT_TEST_PASSED;
}

// ============================================================================
// Test 22 - Timer event signaling during TPL cycling
// ============================================================================

static volatile UINTN  mPeriodicCounter = 0;

static
VOID
EFIAPI
PeriodicCounterCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  mPeriodicCounter++;
}

UNIT_TEST_STATUS
EFIAPI
Test22TimerEventVerify (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   TimerEvent;
  EFI_TPL     OldTpl;
  UINTN       Index;
  UINTN       CounterBefore;

  mPeriodicCounter = 0;

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  PeriodicCounterCallback,
                  NULL,
                  &TimerEvent
                  );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Set periodic timer: fire every 100ms (1,000,000 * 100ns units)
  //
  Status = gBS->SetTimer (TimerEvent, TimerPeriodic, 1000000);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Wait for timer callbacks to fire
  //
  gBS->Stall (StallForTicks (50));
  UT_ASSERT_TRUE (mPeriodicCounter >= 1);

  //
  // Repeat with RaiseTpl/RestoreTpl cycling interleaved
  //
  CounterBefore = mPeriodicCounter;
  for (Index = 0; Index < 100; Index++) {
    OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
    gBS->RestoreTPL (OldTpl);
    gBS->Stall (mTimerPeriodUs);
  }

  UT_ASSERT_TRUE (mPeriodicCounter > CounterBefore);

  gBS->SetTimer (TimerEvent, TimerCancel, 0);
  gBS->CloseEvent (TimerEvent);

  return UNIT_TEST_PASSED;
}

// ============================================================================
// Test 23 - Non-HIGH RestoreTpl after timer interrupt (Scenario 5, 6)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test23NonHighRestoreTplRegression (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_TPL  OldTpl;

  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  //
  // Ensure timer interrupt fires, potentially leaving mask non-zero
  //
  gBS->Stall (StallForTicks (2));

  //
  // Non-HIGH TPL cycle - must NOT check mIsrEntryTplMask
  //
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  gBS->RestoreTPL (OldTpl);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  //
  // Another non-HIGH cycle
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  gBS->RestoreTPL (OldTpl);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  //
  // Verify HIGH cycle still works
  //
  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->RestoreTPL (OldTpl);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  return UNIT_TEST_PASSED;
}
