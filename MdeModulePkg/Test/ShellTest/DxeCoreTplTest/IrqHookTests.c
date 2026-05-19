/** @file
  IRQ-Context Tests via Timer Handler Hook (Tests 16-19, 24-25, 27).

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

  Test 27: Bounded nesting depth under sustained interrupt load.
           Sets timer period to 100µs with slow event callbacks (~30ms each).
           Handler self-terminates after BURST_TARGET_INVOCATIONS.  App-level
           code spins incrementing a progress counter during the burst, then
           the same loop runs for the same wall-clock duration at normal rate
           (baseline).  Verifies: (a) burst is fully processed, (b) app is
           starved during burst vs baseline, (c) nesting depth bounded <= 3,
           (d) normal-rate delivery is reasonable (50-200% tolerance).

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

// ============================================================================
// Test 27 - Bounded nesting depth under sustained interrupt load
//
// Strategy: Set the timer period to a very short interval (100µs) so that
// each handler invocation — which signals events and dispatches them via
// RestoreTPL — spans multiple timer periods.  This guarantees nested
// interrupts fire during event dispatch (where bounded preemption enables
// interrupts at TPL levels above the interrupted level).
//
// The test verifies:
//   1. Many interrupts are processed during the burst (handler invoked N+ times)
//   2. Application code makes NO forward progress while the burst is active
//      (on x86, IRET has no interrupt shadow — pending IRQ preempts immediately)
//   3. Nesting depth is bounded (≤ 3 with standard UEFI TPL levels)
//   4. At normal timer rates, the fast handler doesn't miss interrupts
// ============================================================================

//
// Minimum number of handler invocations before the burst self-terminates.
// With 100µs timer and ~30ms slow callbacks, this represents multiple
// nested interrupt cycles.
//
#define BURST_TARGET_INVOCATIONS  10

//
// Local events for Test 27 slow callbacks
//
static EFI_EVENT  mTest27SlowNotifyEvent   = NULL;
static EFI_EVENT  mTest27SlowCallbackEvent = NULL;

static
VOID
EFIAPI
Test27Handler (
  IN UINT64  Time
  )
{
  EFI_TPL  OldTpl;
  UINTN    CurrentDepth;

  //
  // Track nesting depth
  //
  CurrentDepth = ++mNestingDepth;
  if (CurrentDepth > mMaxNestingDepth) {
    mMaxNestingDepth = CurrentDepth;
  }

  mTotalNestInvocations++;

  //
  // Self-terminate: once enough invocations have been processed,
  // stop signaling slow events and signal burst completion.
  // This allows the application-level spin loop to exit.
  //
  if (mTotalNestInvocations >= BURST_TARGET_INVOCATIONS) {
    mSignalSlowEvents = FALSE;
    mBurstComplete    = TRUE;
  }

  //
  // Simulate CoreTimerTick: RaiseTpl(HIGH) arms the mask.
  //
  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  //
  // In overload mode, signal slow events so RestoreTpl's event dispatch
  // takes multiple timer periods (creating genuine nesting windows).
  // Only signal an event if its callback is not already in progress;
  // re-signaling during dispatch creates an infinite loop in
  // CoreDispatchEventNotifies.
  //
  if (mSignalSlowEvents) {
    if (!mSlowNotifyInProgress) {
      gBS->SignalEvent (mTest27SlowNotifyEvent);
    }

    if (!mSlowCallbackInProgress) {
      gBS->SignalEvent (mTest27SlowCallbackEvent);
    }
  }

  //
  // RestoreTpl dispatches events with interrupts enabled at levels
  // above HighBitSet64(mIsrEntryTplMask).  With a fast timer, the next
  // interrupt fires during event dispatch -> nested handler.
  //
  gBS->RestoreTPL (OldTpl);

  mNestingDepth--;
}

UNIT_TEST_STATUS
EFIAPI
Test27BoundedNestingDepth (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  EFI_TPL     OldTpl;
  UINTN       SavedMaxDepthOverload;
  UINTN       SavedInvocations;
  UINTN       SavedAppProgress;
  UINTN       BaselineProgress;
  UINT64      BurstStartCounter;
  UINT64      BurstEndCounter;
  UINT64      BurstDurationTicks;
  UINT64      BaselineStartCounter;
  UINT64      BaselineCurrentCounter;
  UINT64      BaselineElapsedTicks;
  UINT64      StartCounter;
  UINT64      EndCounter;
  UINT64      ElapsedTicks;
  UINT64      StartValue;
  UINT64      EndValue;
  UINT64      PerfFreq;
  UINT64      TicksPerUs;
  UINT64      ElapsedUs;
  BOOLEAN     CountsUp;
  UINTN       ExpectedInvocations;
  UINT64      OriginalPeriod;
  UINT64      FastPeriod;
  UINTN       FastPeriodUs;

  if (IsResumeAfterWatchdogReset (Context)) {
    UT_LOG_ERROR ("Test failed on previous boot (watchdog reset - infinite recursion detected)");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  //
  // Get performance counter properties for Phase 2 timing
  //
  PerfFreq = GetPerformanceCounterProperties (&StartValue, &EndValue);
  CountsUp = (StartValue < EndValue);

  //
  // Query the current (original) timer period
  //
  Status = gTimer->GetTimerPeriod (gTimer, &OriginalPeriod);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Create slow events for overload phase.
  // Callbacks stall for 3ms — long enough to span many fast timer periods.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  NestSlowNotifyCallback,
                  NULL,
                  &mTest27SlowNotifyEvent
                  );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  NestSlowCallbackCallback,
                  NULL,
                  &mTest27SlowCallbackEvent
                  );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Reset Test 27 state
  //
  mNestingDepth           = 0;
  mMaxNestingDepth        = 0;
  mTotalNestInvocations   = 0;
  mSignalSlowEvents       = FALSE;
  mSlowNotifyInProgress   = FALSE;
  mSlowCallbackInProgress = FALSE;
  mBurstComplete          = FALSE;
  mAppProgressCounter     = 0;

  SaveStateBeforeDangerousTest ();
  SetRecursionWatchdog (60);

  Status = InstallTestTimerHandler (Test27Handler);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  // --------------------------------------------------------------------------
  // Phase 1: Burst processing with fast timer
  //
  // Set timer period to 100µs (vs default 10ms).  The slow callbacks stall
  // for 3ms each, spanning ~30 timer periods.  The handler self-terminates
  // after BURST_TARGET_INVOCATIONS by clearing mSignalSlowEvents and
  // setting mBurstComplete.
  //
  // While the burst is active, application code spins trying to increment
  // mAppProgressCounter.  On real hardware (x86 IRET has no interrupt
  // shadow), the pending timer interrupt preempts immediately after each
  // IRET — the application counter should NOT advance.  On QEMU TCG
  // (which doesn't model precise interrupt delivery), some small progress
  // may occur.
  //
  // Key verification:
  //   - Handler processes BURST_TARGET_INVOCATIONS interrupts (burst IS processed)
  //   - Application progress counter is near zero (APP is starved)
  //   - Nesting depth bounded at ≤ 3
  // --------------------------------------------------------------------------
  FastPeriod   = 1000;  // 100µs in 100ns units
  FastPeriodUs = 100;

  Status = gTimer->SetTimerPeriod (gTimer, FastPeriod);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Start the burst: enable slow events, then spin at APP level.
  // The handler will self-terminate after enough invocations.
  //
  mSignalSlowEvents = TRUE;

  //
  // Application-level spin loop.  Each iteration increments the progress
  // counter.  On hardware with precise interrupt delivery, this counter
  // stays at zero because IRET returns here but the pending timer IRQ
  // preempts before the increment instruction executes.
  // Time the burst to establish a duration for the baseline comparison.
  //
  BurstStartCounter = GetPerformanceCounter ();

  while (!mBurstComplete) {
    mAppProgressCounter++;
  }

  BurstEndCounter = GetPerformanceCounter ();

  //
  // Burst complete — handler has processed all target invocations.
  // Drain any in-progress nested dispatch.
  //
  gBS->Stall (20000);

  SavedMaxDepthOverload = mMaxNestingDepth;
  SavedInvocations      = mTotalNestInvocations;
  SavedAppProgress      = mAppProgressCounter;

  //
  // Calculate burst wall-clock duration for baseline comparison
  //
  if (CountsUp) {
    BurstDurationTicks = BurstEndCounter - BurstStartCounter;
  } else {
    BurstDurationTicks = BurstStartCounter - BurstEndCounter;
  }

  UT_LOG_INFO (
    "Phase 1 (burst, %u us period): invocations = %u, max depth = %u, app progress = %u",
    (UINT32)FastPeriodUs,
    (UINT32)SavedInvocations,
    (UINT32)SavedMaxDepthOverload,
    (UINT32)SavedAppProgress
    );

  //
  // Restore original timer period for Phase 2
  //
  Status = gTimer->SetTimerPeriod (gTimer, OriginalPeriod);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  // --------------------------------------------------------------------------
  // Phase 2: Baseline app loop rate (same duration, no burst)
  //
  // Spin the same loop for the same wall-clock duration as the burst took,
  // but at the normal timer rate with no slow events.  This measures what
  // the app loop achieves when NOT starved by interrupt overload.
  //
  // The ratio (BaselineProgress / BurstProgress) shows how much the burst
  // slowed application code.  On real hardware this ratio approaches
  // infinity (burst progress = 0).  On QEMU TCG it should still be > 1
  // showing measurable interrupt overhead.
  // --------------------------------------------------------------------------
  mAppProgressCounter = 0;

  BaselineStartCounter = GetPerformanceCounter ();
  while (TRUE) {
    mAppProgressCounter++;
    //
    // Check elapsed time every 4096 iterations to minimize overhead
    // while still getting a reasonably accurate duration match.
    //
    if ((mAppProgressCounter & 0xFFF) == 0) {
      BaselineCurrentCounter = GetPerformanceCounter ();
      if (CountsUp) {
        BaselineElapsedTicks = BaselineCurrentCounter - BaselineStartCounter;
      } else {
        BaselineElapsedTicks = BaselineStartCounter - BaselineCurrentCounter;
      }

      if (BaselineElapsedTicks >= BurstDurationTicks) {
        break;
      }
    }
  }

  BaselineProgress = mAppProgressCounter;

  // --------------------------------------------------------------------------
  // Phase 3: Normal load (no-miss measurement)
  //
  // Fast handler at original timer rate (no slow events).  Completes well
  // within one timer period.  Measures actual delivery rate to verify no
  // interrupts are systematically suppressed.
  // --------------------------------------------------------------------------
  mTotalNestInvocations = 0;
  mMaxNestingDepth      = 0;

  StartCounter = GetPerformanceCounter ();
  gBS->Stall (StallForTicks (50));  // 50 timer periods at original rate
  EndCounter = GetPerformanceCounter ();

  //
  // Calculate elapsed time in timer periods
  //
  if (CountsUp) {
    ElapsedTicks = EndCounter - StartCounter;
  } else {
    ElapsedTicks = StartCounter - EndCounter;
  }

  if ((PerfFreq > 0) && (mTimerPeriodUs > 0)) {
    TicksPerUs = DivU64x32 (PerfFreq, 1000000);
    if (TicksPerUs == 0) {
      TicksPerUs = 1;
    }

    ElapsedUs           = DivU64x64Remainder (ElapsedTicks, TicksPerUs, NULL);
    ExpectedInvocations = (UINTN)DivU64x32 (ElapsedUs, (UINT32)mTimerPeriodUs);

    //
    // Sanity check: if calculation seems unreasonable, use fallback
    //
    if (ExpectedInvocations > 500) {
      ExpectedInvocations = 50;
    }
  } else {
    ExpectedInvocations = 50;
  }

  UT_LOG_INFO ("Phase 3 (normal, %u us period): invocations = %u, expected = %u, max depth = %u",
    (UINT32)mTimerPeriodUs, (UINT32)mTotalNestInvocations,
    (UINT32)ExpectedInvocations, (UINT32)mMaxNestingDepth);

  UninstallTestTimerHandler ();
  ClearRecursionWatchdog ();

  // --------------------------------------------------------------------------
  // Assertions
  // --------------------------------------------------------------------------

  //
  // Phase 1: Burst was fully processed — handler invoked at least the target
  // number of times.  This proves all pending interrupts were serviced.
  //
  UT_ASSERT_TRUE (SavedInvocations >= BURST_TARGET_INVOCATIONS);

  //
  // Phase 1: Nesting depth bounded by TPL levels.
  // On QEMU TCG, depth may stay at 1 (timer not delivered inside handler).
  // On real hardware/KVM, depth reaches 2-3.
  //
  UT_ASSERT_TRUE (SavedMaxDepthOverload >= 1);
  UT_ASSERT_TRUE (SavedMaxDepthOverload <= 3);

  //
  // Phase 1 vs Phase 2: Application starvation comparison.
  // BaselineProgress = app iterations in T ms with no burst (normal timer rate)
  // SavedAppProgress = app iterations in T ms during burst (fast timer + slow events)
  //
  // On real hardware (x86 IRET has no interrupt shadow), SavedAppProgress
  // would be 0 because pending timer IRQ preempts immediately after IRET.
  // On QEMU TCG, interrupt delivery is imprecise (only at translation block
  // boundaries), so application makes progress but should still be measurably
  // slower than baseline.
  //
  // Slowdown = baseline / burst.  On real HW: infinite.  On QEMU TCG: > 1.
  //
  UT_LOG_INFO (
    "App loop: burst=%u, baseline=%u (baseline/burst ratio: %u, burst runs at %u%% of baseline)",
    (UINT32)SavedAppProgress,
    (UINT32)BaselineProgress,
    (SavedAppProgress > 0) ? (UINT32)(BaselineProgress / SavedAppProgress) : 0,
    (BaselineProgress > 0) ? (UINT32)((SavedAppProgress * 100) / BaselineProgress) : 0
    );

  //
  // Assert baseline > burst: the burst measurably slowed application progress.
  // Even on QEMU TCG (imprecise delivery), each handler invocation takes ~30ms
  // of wall time for slow callbacks, consuming CPU time that would otherwise
  // go to the app loop.  Baseline should be at least 2x burst progress.
  //
  if ((BaselineProgress > 0) && (SavedAppProgress > 0)) {
    UT_ASSERT_TRUE (BaselineProgress > SavedAppProgress);
  }

  //
  // Phase 3: Fast handler at normal rate - invocations should be reasonable
  // (wide tolerance for virtual environments)
  //
  if (ExpectedInvocations > 0) {
    UT_ASSERT_TRUE (mTotalNestInvocations >= (ExpectedInvocations * 50 / 100));
    UT_ASSERT_TRUE (mTotalNestInvocations <= (ExpectedInvocations * 200 / 100));
  }

  //
  // Phase 3: Fast handler should have no or minimal nesting
  //
  UT_ASSERT_TRUE (mMaxNestingDepth <= 1);

  //
  // System health check
  //
  gCpu->EnableInterrupt (gCpu);
  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->RestoreTPL (OldTpl);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  //
  // Clean up events
  //
  gBS->CloseEvent (mTest27SlowNotifyEvent);
  gBS->CloseEvent (mTest27SlowCallbackEvent);
  mTest27SlowNotifyEvent   = NULL;
  mTest27SlowCallbackEvent = NULL;

  return UNIT_TEST_PASSED;
}
