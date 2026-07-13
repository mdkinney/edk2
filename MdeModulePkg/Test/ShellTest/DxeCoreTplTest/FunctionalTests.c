/** @file
  Functional Tests (Tests 8-16).

  These tests verify that the gIsrEntryTplMask timer recursion fix in the
  DXE Core is effective (prevents infinite recursion) and safe (does not
  regress normal TPL and interrupt behavior under load).

  Test 08: Non-monotonic TPL transitions (HIGH -> APPLICATION -> HIGH).
           Verifies the ISR mask is properly managed across non-monotonic
           TPL transitions where timer interrupts fire in the lowered window.

  Test 09: Timer IRQ during normal event dispatch.
           Uses a slow callback (stalls 2+ timer periods) to guarantee a
           timer interrupt fires DURING event dispatch.  Verifies no crash
           and subsequent operations work normally.

  Test 10: EnableInterrupt at TPL_HIGH_LEVEL (misuse scenario).
           Enabling interrupts while at HIGH allows timer interrupts to fire
           at HIGH - the exact scenario the fix prevents from recursing.
           Uses a watchdog to detect hang.  Verifies system survival.

  Test 11: Sustained RaiseTpl/RestoreTpl - 100,000 iterations.
           Tight loop of RaiseTpl(HIGH)/RestoreTpl with no stall between
           iterations.  Timer interrupts fire asynchronously throughout.
           Verifies no stack overflow, no mask accumulation.

  Test 12: Rapid TPL cycling across all levels - 10,000 iterations.
           Each iteration raises through CALLBACK -> NOTIFY -> HIGH then
           restores back down.  Exercises multi-level nesting where the
           event dispatch loop may fire at each intermediate level.

  Test 13: Timer event signaling during TPL cycling.
           Creates a periodic timer event (100ms period) and verifies
           callbacks fire correctly while interleaving RaiseTpl/RestoreTpl
           cycles.  Confirms the fix does not suppress event dispatch.

  Test 14: Non-HIGH RestoreTpl after timer IRQ (regression test).
           After ensuring a timer interrupt fires (leaving the mask potentially
           non-zero), performs non-HIGH TPL cycles (CALLBACK, NOTIFY) which
           should NOT consult the mask.  Then verifies a HIGH cycle still
           works.  Catches regressions where non-HIGH paths accidentally
           interact with the interrupt mask.

  Test 15: Forced timer recursion - bounded depth verification.
           Uses profiled timer period (1/128th callback cost) to force nested
           re-entry.  Loops RaiseTpl(HIGH)/RestoreTpl until depth 4 is
           observed 3 times (pass) or 60s timeout expires (fail).

  Test 16: Natural timer recursion - bounded depth verification (stall-based).
           Uses profiled timer period (1/128th callback cost), then stalls at
           TPL_APPLICATION for 5ms per iteration instead of explicit
           RaiseTpl/RestoreTpl.  Verifies recursion bounds via natural
           timer delivery.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "DxeCoreTplTest.h"

// ============================================================================
// Test 8 - Non-monotonic TPL transitions (HIGH -> APP -> HIGH)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test08NonMonotonicTpl (
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
// Test 9 - Timer IRQ during normal event dispatch
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test09TimerIrqDuringDispatch (
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
// Test 10 - Enable interrupts at TPL_HIGH (misuse)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test10EnableIrqAtHigh (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_TPL  OldTpl;

  if (IsResumeAfterWatchdogReset (Context)) {
    UT_LOG_ERROR ("Test failed on previous boot (watchdog reset - infinite recursion detected)");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  SaveStateBeforeDangerousTest ();
  SetRecursionWatchdog (30);

  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  UT_ASSERT_FALSE (GetInterruptStateChecked ());

  gCpu->EnableInterrupt (gCpu);
  gBS->Stall (StallForTicks (2));
  gCpu->DisableInterrupt (gCpu);

  gBS->RestoreTPL (OldTpl);

  ClearRecursionWatchdog ();

  //
  // System survived - that's the test. Restore interrupts if needed.
  //
  if (!GetInterruptStateChecked ()) {
    gCpu->EnableInterrupt (gCpu);
  }

  return UNIT_TEST_PASSED;
}

// ============================================================================
// Test 11 - Sustained RaiseTpl/RestoreTpl under timer load (100K iterations)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test11SustainedStress (
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
// Test 12 - Rapid TPL cycling across all levels (10K iterations)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test12RapidTplCycling (
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
// Test 13 - Timer event signaling during TPL cycling
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
Test13TimerEventVerify (
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
// Test 14 - Non-HIGH RestoreTpl after timer interrupt (regression)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test14NonHighRestoreTplRegression (
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
  // Non-HIGH TPL cycle - must NOT check gIsrEntryTplMask
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

// ============================================================================
// Shared infrastructure for Tests 15-16 (aggressive timer recursion)
// ============================================================================
//
// These tests force timer interrupt recursion by programming a fast hardware
// timer period with work-heavy event callbacks.  The setup function profiles
// the callback execution cost and sets the hardware timer period to a test-
// specific fraction of the callback duration (Test 15: 1/128, Test 16: 1/128).
// This ensures:
//   - The timer fires many times during each callback (driving nesting)
//   - The timer does NOT fire so fast that events re-expire before dispatch
//     completes (avoiding livelock)
//
// Without fix: unbounded recursion -> max-depth ASSERT (primary signal),
// then watchdog reset as fallback if ASSERT output is unavailable.
// With fix: bounded nesting (MaxDepth >= TEST_MIN_EXPECTED_DEPTH).
//

//
// Maximum expected nesting depth.  The test environment has only 1 HW
// interrupt source (the timer) and 3 SW TPL levels where event dispatch
// can be interrupted (TPL_APPLICATION, TPL_CALLBACK, TPL_NOTIFY).
// The theoretical full 4-level nesting path is:
//   Depth 1: timer fires at APPLICATION(4)
//   Depth 2: timer fires during CALLBACK(8) dispatch in depth 1
//   Depth 3: timer fires during NOTIFY(16) dispatch in depth 2
//   Depth 4: timer fires during HIGH-1(30) dispatch in depth 3
//
#define TEST_MIN_EXPECTED_DEPTH  4

static volatile UINTN  mRecursionTestCounter = 0;

//
// Work loop iteration count for event callbacks.
//
#define CALLBACK_WORK_ITERATIONS  5000

// Test-specific divisors for profiled timer period.
#define TEST15_PROFILE_DIVISOR    128
#define TEST16_PROFILE_DIVISOR    128

static volatile UINTN  mWorkSink = 0;

//
// Profiled timer period (in 100ns units) set by SetupAggressiveTimer.
//
static UINT64  mProfiledTimerPeriod = 0;

static
VOID
EFIAPI
RecursionTestCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  volatile UINTN  Index;

  mRecursionTestCounter++;
  for (Index = 0; Index < CALLBACK_WORK_ITERATIONS; Index++) {
    mWorkSink += Index;
  }
}

/**
  Profile the callback work loop duration and compute an optimal timer period
  that will fire during callbacks without causing livelock.

  @return  Timer period in 100ns units, or 0 if profiling fails.
**/
static
UINT64
ProfileCallbackDuration (
  IN UINT64  Divisor
  )
{
  UINT64  Start;
  UINT64  End;
  UINT64  ElapsedTicks;
  UINT64  ElapsedUs;
  UINT64  Period100ns;

  //
  // Measure the cost of the callback work loop
  //
  Start = GetPerformanceCounter ();
  {
    volatile UINTN  Index;
    for (Index = 0; Index < CALLBACK_WORK_ITERATIONS; Index++) {
      mWorkSink += Index;
    }
  }
  End = GetPerformanceCounter ();

  ElapsedTicks = GetElapsedTicks (Start, End);
  //
  // Convert to microseconds: ticks / (freq_KHz / 1000) = ticks * 1000 / freq_KHz
  //
  ElapsedUs = (ElapsedTicks * 1000) / mPerfCounterFreqKhz;

  //
  // Set timer period to 1/<Divisor> of callback duration (in 100ns units).
  // A fast period ensures the timer fires many times per callback at each
  // TPL level, maximizing the probability of catching the brief
  // interrupt-enabled window during dispatch.
  //
  if (Divisor == 0) {
    Divisor = TEST15_PROFILE_DIVISOR;
  }

  Period100ns = (ElapsedUs * 10) / Divisor;

  //
  // Clamp: minimum 50 (5us), maximum 10000 (1ms).
  //
  if (Period100ns < 50) {
    Period100ns = 50;
  }

  if (Period100ns > 10000) {
    Period100ns = 10000;
  }

  return Period100ns;
}

/**
  Common setup for Tests 15-16: profile, create events, set aggressive timer.

  @param[out] CallbackEvent   Created TPL_CALLBACK event.
  @param[out] NotifyEvent     Created TPL_NOTIFY event.
  @param[out] OriginalPeriod  Saved original timer period for restore.

  @retval EFI_SUCCESS   Setup complete.
  @retval other         A boot service call failed.
**/
static
EFI_STATUS
SetupAggressiveTimer (
  OUT EFI_EVENT  *CallbackEvent,
  OUT EFI_EVENT  *NotifyEvent,
  OUT UINT64     *OriginalPeriod,
  IN  UINT64     ProfileDivisor
  )
{
  EFI_STATUS  Status;

  Status = gTimer->GetTimerPeriod (gTimer, OriginalPeriod);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Profile callback duration to determine optimal timer period
  //
  mProfiledTimerPeriod = ProfileCallbackDuration (ProfileDivisor);

  mRecursionTestCounter = 0;

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  RecursionTestCallback,
                  NULL,
                  CallbackEvent
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  RecursionTestCallback,
                  NULL,
                  NotifyEvent
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (*CallbackEvent);
    return Status;
  }

  //
  // Set periodic timer events aggressively for cross-architecture parity.
  // Stress tests are intentionally configured to maximize recursive pressure
  // so no-fix builds hit depth overflow quickly while fixed builds remain
  // bounded.
  //
  {
    UINT64  EventPeriod;

    EventPeriod = 1;
    UT_LOG_INFO ("Stress mode: aggressive (event period=1)");

    Status = gBS->SetTimer (*CallbackEvent, TimerPeriodic, EventPeriod);
    if (EFI_ERROR (Status)) {
      gBS->CloseEvent (*NotifyEvent);
      gBS->CloseEvent (*CallbackEvent);
      return Status;
    }

    Status = gBS->SetTimer (*NotifyEvent, TimerPeriodic, EventPeriod);
    if (EFI_ERROR (Status)) {
      gBS->SetTimer (*CallbackEvent, TimerCancel, 0);
      gBS->CloseEvent (*NotifyEvent);
      gBS->CloseEvent (*CallbackEvent);
      return Status;
    }
  }

  //
  // Reprogram hardware timer to the profiled period.
  //
  Status = gTimer->SetTimerPeriod (gTimer, mProfiledTimerPeriod);
  if (EFI_ERROR (Status)) {
    gBS->SetTimer (*CallbackEvent, TimerCancel, 0);
    gBS->SetTimer (*NotifyEvent, TimerCancel, 0);
    gBS->CloseEvent (*NotifyEvent);
    gBS->CloseEvent (*CallbackEvent);
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Common teardown for Tests 15-16: restore timer, close events.
**/
static
VOID
TeardownAggressiveTimer (
  IN EFI_EVENT  CallbackEvent,
  IN EFI_EVENT  NotifyEvent,
  IN UINT64     OriginalPeriod
  )
{
  gTimer->SetTimerPeriod (gTimer, OriginalPeriod);
  gBS->SetTimer (CallbackEvent, TimerCancel, 0);
  gBS->CloseEvent (CallbackEvent);
  gBS->SetTimer (NotifyEvent, TimerCancel, 0);
  gBS->CloseEvent (NotifyEvent);
}

//
// Number of times each nesting level must be observed to pass.
//
#define DEPTH_OBSERVATION_COUNT  10

//
// Timeout in seconds for depth observation loops.
//
#define DEPTH_OBSERVATION_TIMEOUT_SEC  60

//
// TPL values corresponding to each nesting depth level.
//
#define DEPTH_LEVEL_COUNT  4

static CONST EFI_TPL  mDepthTplLevels[DEPTH_LEVEL_COUNT] = {
  TPL_APPLICATION,      // Depth 1
  TPL_CALLBACK,         // Depth 2
  TPL_NOTIFY,           // Depth 3
  TPL_HIGH_LEVEL - 1    // Depth 4
};

/**
  Check if all nesting depth levels have been observed DEPTH_OBSERVATION_COUNT
  times.

  @param[in] EntriesBefore  Array of EntriesAtTpl values at test start.

  @retval TRUE   All levels observed enough times.
  @retval FALSE  Not yet observed enough times.
**/
static
BOOLEAN
AllDepthLevelsObserved (
  IN CONST UINTN  *EntriesBefore
  )
{
  UINTN  Level;
  UINTN  TplIndex;
  UINTN  NewEntries;

  if (gTimerTickDiag == NULL) {
    return FALSE;
  }

  for (Level = 0; Level < DEPTH_LEVEL_COUNT; Level++) {
    TplIndex   = mDepthTplLevels[Level];
    NewEntries = gTimerTickDiag->EntriesAtTpl[TplIndex] - EntriesBefore[Level];
    if (NewEntries < DEPTH_OBSERVATION_COUNT) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Log the observation counts for all nesting levels.

  @param[in] EntriesBefore  Array of EntriesAtTpl values at test start.
  @param[in] IterationCount Number of loop iterations performed.
**/
static
VOID
LogDepthObservations (
  IN CONST UINTN  *EntriesBefore,
  IN UINTN        IterationCount
  )
{
  UINTN  Level;
  UINTN  TplIndex;
  UINTN  NewEntries;

  if (gTimerTickDiag == NULL) {
    return;
  }

  for (Level = 0; Level < DEPTH_LEVEL_COUNT; Level++) {
    TplIndex   = mDepthTplLevels[Level];
    NewEntries = gTimerTickDiag->EntriesAtTpl[TplIndex] - EntriesBefore[Level];
    UT_LOG_INFO (
      "  Depth %u (TPL %u): %u/%u observations",
      (UINT32)(Level + 1),
      (UINT32)TplIndex,
      (UINT32)NewEntries,
      (UINT32)DEPTH_OBSERVATION_COUNT
      );
  }

  UT_LOG_INFO ("  Loop iterations: %lu", (UINT64)IterationCount);
}

/**
  Check pass/fail for depth observations.  All 4 depth levels must have
  been observed at least DEPTH_OBSERVATION_COUNT times.

  @param[in] EntriesBefore  Array of EntriesAtTpl values at test start.

  @retval UNIT_TEST_PASSED              All levels met the threshold.
  @retval UNIT_TEST_ERROR_TEST_FAILED   At least one level did not meet threshold.
**/
static
UNIT_TEST_STATUS
CheckDepthObservationResult (
  IN CONST UINTN  *EntriesBefore
  )
{
  UINTN             Level;
  UINTN             TplIndex;
  UINTN             NewEntries;
  UNIT_TEST_STATUS  Status;

  if (gTimerTickDiag == NULL) {
    UT_LOG_ERROR ("Timer diagnostics config table not available");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Status = UNIT_TEST_PASSED;
  for (Level = 0; Level < DEPTH_LEVEL_COUNT; Level++) {
    TplIndex   = mDepthTplLevels[Level];
    NewEntries = gTimerTickDiag->EntriesAtTpl[TplIndex] - EntriesBefore[Level];
    if (NewEntries < DEPTH_OBSERVATION_COUNT) {
      UT_LOG_ERROR (
        "Depth %u (TPL %u): observed %u/%u times within %u seconds (timeout)",
        (UINT32)(Level + 1),
        (UINT32)TplIndex,
        (UINT32)NewEntries,
        (UINT32)DEPTH_OBSERVATION_COUNT,
        (UINT32)DEPTH_OBSERVATION_TIMEOUT_SEC
        );
      Status = UNIT_TEST_ERROR_TEST_FAILED;
    }
  }

  return Status;
}

// ============================================================================
// Test 15 - Forced timer recursion: bounded depth verification
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test15ForcedTimerRecursion (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS        Status;
  EFI_EVENT         CallbackEvent;
  EFI_EVENT         NotifyEvent;
  UINT64            OriginalPeriod;
  UINTN             EntriesBefore[DEPTH_LEVEL_COUNT];
  UINTN             IterationCount;
  UINTN             Level;
  UINT64            StartTick;
  UINT64            ElapsedTicks;
  UINT64            ElapsedSec;
  UNIT_TEST_STATUS  TestResult;

  if (IsResumeAfterWatchdogReset (Context)) {
    UT_LOG_ERROR ("Watchdog reset - infinite recursion detected (EXPECTED without fix)");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  SaveStateBeforeDangerousTest ();

  Status = SetupAggressiveTimer (
             &CallbackEvent,
             &NotifyEvent,
             &OriginalPeriod,
             TEST15_PROFILE_DIVISOR
             );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  UT_LOG_INFO (
    "  Profiled timer period: %lu (100ns units), callback work: %u iterations, divisor: 1/%u",
    mProfiledTimerPeriod,
    (UINT32)CALLBACK_WORK_ITERATIONS,
    (UINT32)TEST15_PROFILE_DIVISOR
    );

  SetRecursionWatchdog (DEPTH_OBSERVATION_TIMEOUT_SEC + 30);

  //
  // Record starting EntriesAtTpl counts for all nesting levels.
  //
  for (Level = 0; Level < DEPTH_LEVEL_COUNT; Level++) {
    EntriesBefore[Level] = (gTimerTickDiag != NULL)
                           ? gTimerTickDiag->EntriesAtTpl[mDepthTplLevels[Level]]
                           : 0;
  }

  //
  // Loop RaiseTpl(HIGH)/RestoreTpl(APPLICATION) until all nesting levels
  // have each been entered DEPTH_OBSERVATION_COUNT new times, or timeout.
  //
  IterationCount = 0;
  StartTick      = GetPerformanceCounter ();

  while (!AllDepthLevelsObserved (EntriesBefore)) {
    EFI_TPL  OldTpl;

    OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
    gBS->RestoreTPL (OldTpl);
    IterationCount++;

    ElapsedTicks = GetElapsedTicks (StartTick, GetPerformanceCounter ());
    ElapsedSec   = (ElapsedTicks * 1000 / mPerfCounterFreqKhz) / 1000000;
    if (ElapsedSec >= DEPTH_OBSERVATION_TIMEOUT_SEC) {
      break;
    }
  }

  TeardownAggressiveTimer (CallbackEvent, NotifyEvent, OriginalPeriod);
  ClearRecursionWatchdog ();

  UT_ASSERT_TRUE (GetInterruptStateChecked ());
  UT_ASSERT_TRUE (mRecursionTestCounter > 0);

  LogDepthObservations (EntriesBefore, IterationCount);
  TestResult = CheckDepthObservationResult (EntriesBefore);

  return TestResult;
}

// ============================================================================
// Test 16 - Natural timer recursion: bounded depth verification (stall-based)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test16NaturalTimerRecursion (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS        Status;
  EFI_EVENT         CallbackEvent;
  EFI_EVENT         NotifyEvent;
  UINT64            OriginalPeriod;
  UINTN             EntriesBefore[DEPTH_LEVEL_COUNT];
  UINTN             IterationCount;
  UINTN             Level;
  UINT64            StartTick;
  UINT64            ElapsedTicks;
  UINT64            ElapsedSec;
  UNIT_TEST_STATUS  TestResult;

  if (IsResumeAfterWatchdogReset (Context)) {
    UT_LOG_ERROR ("Watchdog reset - infinite recursion detected (EXPECTED without fix)");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  SaveStateBeforeDangerousTest ();

  Status = SetupAggressiveTimer (
             &CallbackEvent,
             &NotifyEvent,
             &OriginalPeriod,
             TEST16_PROFILE_DIVISOR
             );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  UT_LOG_INFO (
    "  Profiled timer period: %lu (100ns units), callback work: %u iterations, divisor: 1/%u",
    mProfiledTimerPeriod,
    (UINT32)CALLBACK_WORK_ITERATIONS,
    (UINT32)TEST16_PROFILE_DIVISOR
    );

  SetRecursionWatchdog (DEPTH_OBSERVATION_TIMEOUT_SEC + 30);

  for (Level = 0; Level < DEPTH_LEVEL_COUNT; Level++) {
    EntriesBefore[Level] = (gTimerTickDiag != NULL)
                           ? gTimerTickDiag->EntriesAtTpl[mDepthTplLevels[Level]]
                           : 0;
  }

  //
  // Stall at TPL_APPLICATION for 5ms per iteration.  Timer interrupts fire
  // naturally via CoreTimerTick -> CoreReleaseLock -> RestoreTpl.
  //
  IterationCount = 0;
  StartTick      = GetPerformanceCounter ();

  while (!AllDepthLevelsObserved (EntriesBefore)) {
    gBS->Stall (5000);
    IterationCount++;

    ElapsedTicks = GetElapsedTicks (StartTick, GetPerformanceCounter ());
    ElapsedSec   = (ElapsedTicks * 1000 / mPerfCounterFreqKhz) / 1000000;
    if (ElapsedSec >= DEPTH_OBSERVATION_TIMEOUT_SEC) {
      break;
    }
  }

  TeardownAggressiveTimer (CallbackEvent, NotifyEvent, OriginalPeriod);
  ClearRecursionWatchdog ();

  UT_ASSERT_TRUE (GetInterruptStateChecked ());
  UT_ASSERT_TRUE (mRecursionTestCounter > 0);

  LogDepthObservations (EntriesBefore, IterationCount);
  TestResult = CheckDepthObservationResult (EntriesBefore);

  return TestResult;
}
