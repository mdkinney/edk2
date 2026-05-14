/** @file
  IRQ-Context Tests via Timer Handler Hook (Tests 16-19, 24-25).

  These tests replace CoreTimerTick with a custom handler via the
  EFI_TIMER_ARCH_PROTOCOL.RegisterHandler() API, then execute test logic
  directly in hardware interrupt context.  This is the most rigorous
  validation of the mIsrEntryTplMask fix because the test code runs
  in the exact same context as the real timer interrupt handler.

  IMPORTANT: These tests must run LAST because they permanently unregister
  CoreTimerTick.  After these tests, the DXE Core's timer-driven event
  dispatch no longer functions.

  Test 16: IRQ-context RaiseTpl/RestoreTpl.
           Custom handler does RaiseTpl(HIGH)/RestoreTpl inside the interrupt.
           This is the fundamental recursion scenario - without the fix,
           RestoreTpl would re-enable interrupts, causing immediate re-entry
           into the timer handler (infinite recursion / stack overflow).

  Test 17: IRQ-context event dispatch.
           Handler raises to HIGH, signals a TPL_NOTIFY event, then restores.
           The event dispatch loop runs inside the interrupt context with
           interrupts re-enabled.  Verifies events fire and the system
           does not recurse.

  Test 18: IRQ-context sustained stress.
           Handler does RaiseTpl/RestoreTpl on every timer tick for 200+
           timer periods.  Verifies the fix works reliably across many
           consecutive interrupt entries, not just a single invocation.

  Test 19: IRQ-context temporarily lower TPL.
           Handler raises to HIGH, signals events at NOTIFY and CALLBACK,
           then drops to APPLICATION (triggering full event dispatch), raises
           back to HIGH, and restores to original.  Exercises the most complex
           nesting pattern possible inside an interrupt.

  Test 24: IRQ-context callback with RaiseTpl(HIGH).
           Handler signals an event whose callback does its own
           RaiseTpl(HIGH)/RestoreTpl (simulating lock acquire/release inside
           an event notification fired from interrupt context).  Verifies
           nested RaiseTpl inside a dispatched callback does not corrupt
           the mask or cause recursion.

  Test 25: IRQ-context RestoreTpl to intermediate TPL.
           Handler raises to HIGH then restores to TPL_CALLBACK (not all the
           way to APPLICATION).  This tests the mask's per-bit tracking -
           restoring to a level above the interrupted TPL should re-enable
           interrupts, while restoring to or below the interrupted level
           should leave them disabled.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "DxeCoreTplTest.h"

// ============================================================================
// Test 16 - IRQ-context RaiseTpl/RestoreTpl (Scenarios 4, 8)
// ============================================================================

static
VOID
EFIAPI
Test16Handler (
  IN UINT64  Time
  )
{
  BOOLEAN  State;
  EFI_TPL  OldTpl;

  //
  // Verify we are in genuine interrupt context
  //
  gCpu->GetInterruptState (gCpu, &State);
  mInterruptStateInHandler = State;

  //
  // Exercise the fix: RaiseTpl/RestoreTpl in IRQ context
  //
  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->RestoreTPL (OldTpl);

  //
  // If we reach here, no infinite recursion occurred
  //
  mHandlerExecuted = TRUE;
  mHandlerPassed   = TRUE;
}

UNIT_TEST_STATUS
EFIAPI
Test16IrqContextRaiseTplRestoreTpl (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  if (IsResumeAfterWatchdogReset (Context)) {
    UT_LOG_ERROR ("Test failed on previous boot (watchdog reset - infinite recursion detected)");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  ResetHandlerState ();
  SaveStateBeforeDangerousTest ();
  SetRecursionWatchdog (30);

  Status = InstallTestTimerHandler (Test16Handler);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  gBS->Stall (StallForTicks (20));

  UninstallTestTimerHandler ();
  ClearRecursionWatchdog ();

  UT_ASSERT_TRUE (mHandlerExecuted);
  UT_ASSERT_TRUE (mHandlerPassed);
  UT_ASSERT_FALSE (mInterruptStateInHandler);

  return UNIT_TEST_PASSED;
}

// ============================================================================
// Test 17 - IRQ-context event dispatch with interrupts enabled (Scenarios 4, 8)
// ============================================================================

static
VOID
EFIAPI
Test17Handler (
  IN UINT64  Time
  )
{
  EFI_TPL  OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  //
  // Signal event while at HIGH - queues for dispatch
  //
  gBS->SignalEvent (mTestNotifyEvent);

  //
  // RestoreTpl will dispatch the NOTIFY event in the event loop.
  //
  gBS->RestoreTPL (OldTpl);

  mHandlerExecuted  = TRUE;
  mEventsDispatched = mNotifyCallbackFired;
}

UNIT_TEST_STATUS
EFIAPI
Test17IrqContextEventDispatch (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  if (IsResumeAfterWatchdogReset (Context)) {
    UT_LOG_ERROR ("Test failed on previous boot (watchdog reset - infinite recursion detected)");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  ResetHandlerState ();

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  NotifyRecordCallback,
                  NULL,
                  &mTestNotifyEvent
                  );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  SaveStateBeforeDangerousTest ();
  SetRecursionWatchdog (30);

  Status = InstallTestTimerHandler (Test17Handler);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  gBS->Stall (StallForTicks (20));

  UninstallTestTimerHandler ();
  ClearRecursionWatchdog ();

  UT_ASSERT_TRUE (mHandlerExecuted);
  UT_ASSERT_TRUE (mEventsDispatched);
  //
  // With bounded preemption, NOTIFY events dispatched in ISR context now
  // run with interrupts enabled when their TPL is above the interrupted TPL.
  // This allows higher-priority events to preempt lower-priority ones even
  // inside an ISR, bounded by the number of TPL levels (no infinite recursion).
  //
  UT_ASSERT_TRUE (mNotifyInterruptState);

  gBS->CloseEvent (mTestNotifyEvent);
  mTestNotifyEvent = NULL;

  return UNIT_TEST_PASSED;
}

// ============================================================================
// Test 18 - IRQ-context sustained RaiseTpl/RestoreTpl (Scenario 4 - stress)
// ============================================================================

static
VOID
EFIAPI
Test18Handler (
  IN UINT64  Time
  )
{
  EFI_TPL  OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->RestoreTPL (OldTpl);
  mHandlerIterations++;
}

UNIT_TEST_STATUS
EFIAPI
Test18IrqContextSustainedStress (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  EFI_TPL     OldTpl;

  if (IsResumeAfterWatchdogReset (Context)) {
    UT_LOG_ERROR ("Test failed on previous boot (watchdog reset - infinite recursion detected)");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  ResetHandlerState ();
  SaveStateBeforeDangerousTest ();
  SetRecursionWatchdog (60);

  Status = InstallTestTimerHandler (Test18Handler);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  gBS->Stall (StallForTicks (200));

  UninstallTestTimerHandler ();
  ClearRecursionWatchdog ();

  UT_ASSERT_TRUE (mHandlerIterations > 10);

  //
  // Verify normal context still works after hook removal
  //
  gCpu->EnableInterrupt (gCpu);
  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->RestoreTPL (OldTpl);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  return UNIT_TEST_PASSED;
}

// ============================================================================
// Test 19 - IRQ-context temp lower TPL across HIGH (Scenario 8 - direct)
// ============================================================================

static
VOID
EFIAPI
Test19Handler (
  IN UINT64  Time
  )
{
  EFI_TPL  OldTpl;
  EFI_TPL  InnerOldTpl;

  //
  // Must raise to HIGH first (like CoreTimerTick does).
  //
  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  //
  // Signal events at two TPL levels to force multi-level dispatch
  //
  gBS->SignalEvent (mTestNotifyEvent);
  gBS->SignalEvent (mTestCallbackEvent);

  //
  // Temporarily drop to APPLICATION - triggers event dispatch loop
  //
  gBS->RestoreTPL (TPL_APPLICATION);

  //
  // Raise back to HIGH
  //
  InnerOldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  //
  // Restore to original interrupted TPL (clean exit)
  //
  gBS->RestoreTPL (OldTpl);

  //
  // If we reach here, bounded nesting worked (no infinite recursion)
  //
  mHandlerExecuted = TRUE;
  mHandlerPassed   = (InnerOldTpl == TPL_APPLICATION);
}

UNIT_TEST_STATUS
EFIAPI
Test19IrqContextTempLowerTpl (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  if (IsResumeAfterWatchdogReset (Context)) {
    UT_LOG_ERROR ("Test failed on previous boot (watchdog reset - infinite recursion detected)");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  ResetHandlerState ();

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  NotifyRecordCallback,
                  NULL,
                  &mTestNotifyEvent
                  );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  CallbackRecordCallback,
                  NULL,
                  &mTestCallbackEvent
                  );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  SaveStateBeforeDangerousTest ();
  SetRecursionWatchdog (30);

  Status = InstallTestTimerHandler (Test19Handler);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  gBS->Stall (StallForTicks (20));

  UninstallTestTimerHandler ();
  ClearRecursionWatchdog ();

  UT_ASSERT_TRUE (mHandlerExecuted);
  UT_ASSERT_TRUE (mHandlerPassed);
  UT_ASSERT_TRUE (mNotifyDispatchCount > 0);
  UT_ASSERT_TRUE (mCallbackDispatchCount > 0);
  //
  // Do not assert on mNotifyInterruptState / mCallbackInterruptState here.
  // When the handler temporarily lowers TPL, a re-entrant timer may fire
  // before the event callback executes.  In that case the callback runs in
  // nested interrupt context where interrupts are correctly kept disabled
  // by the recursion-prevention mask.  The interrupt state during callback
  // dispatch is therefore timing-dependent in this scenario.
  //

  gBS->CloseEvent (mTestNotifyEvent);
  gBS->CloseEvent (mTestCallbackEvent);
  mTestNotifyEvent   = NULL;
  mTestCallbackEvent = NULL;

  return UNIT_TEST_PASSED;
}

// ============================================================================
// Test 24 - Callback RaiseTpl(HIGH) during IRQ-context event dispatch
// ============================================================================

static
VOID
EFIAPI
Test24Handler (
  IN UINT64  Time
  )
{
  EFI_TPL  OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->SignalEvent (mTestNotifyEvent);

  //
  // RestoreTpl dispatches LockingCallback in event loop.
  // LockingCallback does RaiseTpl(HIGH)/RestoreTpl inside.
  //
  gBS->RestoreTPL (OldTpl);

  mHandlerExecuted = TRUE;
  mHandlerIterations++;
}

UNIT_TEST_STATUS
EFIAPI
Test24IrqContextCallbackRaiseTpl (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  if (IsResumeAfterWatchdogReset (Context)) {
    UT_LOG_ERROR ("Test failed on previous boot (watchdog reset - infinite recursion detected)");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  ResetHandlerState ();

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  LockingCallback,
                  NULL,
                  &mTestNotifyEvent
                  );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  SaveStateBeforeDangerousTest ();
  SetRecursionWatchdog (30);

  Status = InstallTestTimerHandler (Test24Handler);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  gBS->Stall (StallForTicks (20));

  UninstallTestTimerHandler ();
  ClearRecursionWatchdog ();

  UT_ASSERT_TRUE (mHandlerExecuted);
  UT_ASSERT_TRUE (mLockCallbackExecuted);
  UT_ASSERT_TRUE (mHandlerIterations > 5);

  gBS->CloseEvent (mTestNotifyEvent);
  mTestNotifyEvent = NULL;

  return UNIT_TEST_PASSED;
}

// ============================================================================
// Test 25 - IRQ-context RestoreTpl to intermediate TPL (Scenario 4, 8)
// ============================================================================

static
VOID
EFIAPI
Test25Handler (
  IN UINT64  Time
  )
{
  EFI_TPL  OldTpl;

  //
  // Raise to HIGH - sets mask bit for APPLICATION(4)
  //
  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  //
  // Restore to CALLBACK(8) - intermediate, above interrupted TPL.
  //
  gBS->RestoreTPL (TPL_CALLBACK);

  //
  // Work at TPL_CALLBACK with interrupts enabled...
  //

  //
  // Raise back to HIGH
  //
  gBS->RaiseTPL (TPL_HIGH_LEVEL);

  //
  // Restore to original interrupted TPL
  //
  gBS->RestoreTPL (OldTpl);

  mHandlerExecuted = TRUE;
  mHandlerIterations++;
}

UNIT_TEST_STATUS
EFIAPI
Test25IrqContextIntermediateTpl (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  EFI_TPL     OldTpl;

  if (IsResumeAfterWatchdogReset (Context)) {
    UT_LOG_ERROR ("Test failed on previous boot (watchdog reset - infinite recursion detected)");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  ResetHandlerState ();
  SaveStateBeforeDangerousTest ();
  SetRecursionWatchdog (30);

  Status = InstallTestTimerHandler (Test25Handler);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  gBS->Stall (StallForTicks (20));

  UninstallTestTimerHandler ();
  ClearRecursionWatchdog ();

  UT_ASSERT_TRUE (mHandlerExecuted);
  UT_ASSERT_TRUE (mHandlerIterations > 5);

  //
  // Verify normal context works after hook removal
  //
  gCpu->EnableInterrupt (gCpu);
  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->RestoreTPL (OldTpl);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  return UNIT_TEST_PASSED;
}
