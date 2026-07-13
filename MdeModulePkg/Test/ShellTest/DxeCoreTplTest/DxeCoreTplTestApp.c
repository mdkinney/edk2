/** @file
  DXE Core TPL Timer Interrupt Recursion Test Application - Main Runner.

  This UEFI Shell application verifies that the gIsrEntryTplMask-based fix
  in CoreRestoreTpl (MdeModulePkg/Core/Dxe/Event/Tpl.c) prevents infinite
  recursion from timer interrupts while preserving normal TPL and interrupt
  behavior per the PI and UEFI specifications.

  This file contains:
  - Global variable definitions shared across all test source files
  - Helper functions (watchdog, interrupt state)
  - Event callback functions used by multiple test suites
  - Performance counter timing infrastructure (portable across IA32/X64/AARCH64)
  - The main test framework entry point that registers all 16 test cases
    organized into 2 suites and measures total execution time

  Tests cover 16 scenarios from the TimerInterruptRecursionAnalysis.md:
  - Tests 1-7:   Spec conformance (PI/UEFI TPL and interrupt semantics)
  - Tests 8-16:  Functional (recursion fix effectiveness and safety)

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "DxeCoreTplTest.h"

#define UNIT_TEST_NAME     "DXE Core TPL Timer Interrupt Recursion Test"
#define UNIT_TEST_VERSION  "1.0"

#define MAX_TEST_RESULTS  16

typedef struct {
  UINTN      TestNum;
  UINT64     ElapsedMs;
  BOOLEAN    Passed;
} TEST_RESULT_ENTRY;

static TEST_RESULT_ENTRY  mTestResults[MAX_TEST_RESULTS];
static UINTN              mTestResultCount = 0;

// Minimal mirror of UnitTestFrameworkPkg private types used to read test results.
typedef struct {
  CHAR8               *Description;
  CHAR8               *Name;
  CHAR8               *Log;
  UINT32              FailureType;
  CHAR8               FailureMessage[512];
  UINT8               Fingerprint[sizeof (UINT32)];
  UNIT_TEST_STATUS    Result;
  VOID                *RunTest;
  VOID                *Prerequisite;
  VOID                *CleanUp;
  VOID                *Context;
  VOID                *ParentSuite;
} APP_UNIT_TEST;

typedef struct {
  LIST_ENTRY       Entry;
  APP_UNIT_TEST    UT;
} APP_UNIT_TEST_LIST_ENTRY;

typedef struct {
  UINTN         NumTests;
  CHAR8         *Title;
  CHAR8         *Name;
  UINT8         Fingerprint[sizeof (UINT32)];
  VOID          *Setup;
  VOID          *Teardown;
  LIST_ENTRY    TestCaseList;
  VOID          *ParentFramework;
} APP_UNIT_TEST_SUITE;

typedef struct {
  LIST_ENTRY             Entry;
  APP_UNIT_TEST_SUITE    UTS;
} APP_UNIT_TEST_SUITE_LIST_ENTRY;

typedef struct {
  CHAR8         *Title;
  CHAR8         *ShortTitle;
  CHAR8         *VersionString;
  CHAR8         *Log;
  UINT8         Fingerprint[sizeof (UINT32)];
  LIST_ENTRY    TestSuiteList;
  EFI_TIME      StartTime;
  EFI_TIME      EndTime;
  VOID          *CurrentTest;
  VOID          *SavedState;
} APP_UNIT_TEST_FRAMEWORK;

//
// Protocol instances
//
EFI_CPU_ARCH_PROTOCOL    *gCpu   = NULL;
EFI_TIMER_ARCH_PROTOCOL  *gTimer = NULL;

//
// Pointer to DxeCore's timer tick diagnostics (from EFI config table)
//
CORE_TIMER_TICK_DIAGNOSTICS  *gTimerTickDiag = NULL;

//
// Timer period in microseconds (computed at init)
//
UINTN  mTimerPeriodUs = 0;

//
// Shared volatile state for event callbacks (Tests 7-8)
//
volatile BOOLEAN  mNotifyCallbackFired    = FALSE;
volatile BOOLEAN  mNotifyInterruptState   = FALSE;
volatile UINTN    mNotifyDispatchCount    = 0;
volatile BOOLEAN  mCallbackInterruptState = FALSE;
volatile UINTN    mCallbackDispatchCount  = 0;
volatile BOOLEAN  mCallbackSurvived       = FALSE;

//
// Timing infrastructure
//
UINT64           mPerfCounterFreqKhz  = 0;
BOOLEAN          mPerfCounterCountsUp = TRUE;
static UINT64    mPerfCounterRange    = MAX_UINT64; // EndValue - StartValue + 1
static UINT64    mPerfCounterWrapMs   = MAX_UINT64; // ms before counter wraps
static UINT64    mTestStartCounter    = 0;
static UINT64    mTotalStartCounter   = 0;
static UINTN     mTestIndex           = 0;
static EFI_TIME  mTestStartTime;                   // wall-clock backup
static EFI_TIME  mTotalStartTime;

// ============================================================================
// Helper Functions
// ============================================================================

BOOLEAN
GetInterruptStateChecked (
  VOID
  )
{
  BOOLEAN     State;
  EFI_STATUS  Status;

  State  = FALSE;
  Status = gCpu->GetInterruptState (gCpu, &State);
  ASSERT (!EFI_ERROR (Status));
  return State;
}

UINTN
StallForTicks (
  IN UINTN  TickCount
  )
{
  return mTimerPeriodUs * (TickCount + 1);
}

VOID
SetRecursionWatchdog (
  IN UINTN  TimeoutSeconds
  )
{
  gBS->SetWatchdogTimer (TimeoutSeconds, 0, 0, NULL);
}

VOID
ClearRecursionWatchdog (
  VOID
  )
{
  gBS->SetWatchdogTimer (0, 0, 0, NULL);
}

//
// Sentinel value saved before dangerous test sections.
// If the system resets, the framework restores this as the test Context.
//
static UINT8  mWatchdogSentinel = 0xAA;

BOOLEAN
IsResumeAfterWatchdogReset (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return (Context != NULL);
}

VOID
SaveStateBeforeDangerousTest (
  VOID
  )
{
  SaveFrameworkState (&mWatchdogSentinel, sizeof (mWatchdogSentinel));
}

// ============================================================================
// Event Callbacks (shared across test files)
// ============================================================================

VOID
EFIAPI
NotifyRecordCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  mNotifyCallbackFired  = TRUE;
  mNotifyInterruptState = GetInterruptStateChecked ();
  mNotifyDispatchCount++;
}

VOID
EFIAPI
CallbackRecordCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  mCallbackInterruptState = GetInterruptStateChecked ();
  mCallbackDispatchCount++;
}

VOID
EFIAPI
SlowNotifyCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  mCallbackInterruptState = GetInterruptStateChecked ();
  //
  // Stall >= 2 timer periods - guarantees timer interrupt fires
  // DURING this dispatch, exercising Scenario 5
  //
  gBS->Stall (StallForTicks (2));
  mCallbackSurvived = TRUE;
}

// ============================================================================
// Timing Infrastructure
// ============================================================================

UINT64
GetElapsedTicks (
  IN UINT64  Start,
  IN UINT64  End
  )
{
  //
  // Handle wrap-around for narrow counters (e.g. 24-bit ACPI PM Timer).
  // If the counter wrapped (End < Start for count-up, End > Start for
  // count-down), add the counter range to get the correct elapsed value.
  //
  if (mPerfCounterCountsUp) {
    if (End >= Start) {
      return End - Start;
    } else {
      return (mPerfCounterRange - Start) + End;
    }
  } else {
    if (Start >= End) {
      return Start - End;
    } else {
      return (mPerfCounterRange - End) + Start;
    }
  }
}

/**
  Compute wall-clock elapsed milliseconds between two EFI_TIME values.
  Handles minute/hour/day boundaries. Accurate for durations < 24 hours.
**/
static
UINT64
GetWallClockElapsedMs (
  IN EFI_TIME  *Start,
  IN EFI_TIME  *End
  )
{
  UINT64  StartSec;
  UINT64  EndSec;

  StartSec = (UINT64)Start->Hour * 3600 + (UINT64)Start->Minute * 60 + Start->Second;
  EndSec   = (UINT64)End->Hour * 3600 + (UINT64)End->Minute * 60 + End->Second;

  //
  // Handle day boundary wrap (test started before midnight, ended after)
  //
  if (EndSec < StartSec) {
    EndSec += 86400;
  }

  return (EndSec - StartSec) * 1000;
}

static
VOID
CalibratePerformanceCounter (
  VOID
  )
{
  UINT64  StartValue;
  UINT64  EndValue;
  UINT64  Freq;

  Freq                 = GetPerformanceCounterProperties (&StartValue, &EndValue);
  mPerfCounterCountsUp = (StartValue < EndValue);
  mPerfCounterFreqKhz  = Freq / 1000;  // Hz to KHz (ticks per ms)
  if (mPerfCounterFreqKhz == 0) {
    mPerfCounterFreqKhz = 1;
  }

  //
  // Compute counter range for wrap-around handling.
  // For a 24-bit count-up counter: StartValue=0, EndValue=0xFFFFFF
  //   -> range = 0xFFFFFF - 0 + 1 = 0x1000000 (16,777,216)
  // For a 64-bit counter: EndValue - StartValue + 1 overflows to 0.
  //   Detect this and treat as "never wraps".
  //
  if (mPerfCounterCountsUp) {
    mPerfCounterRange = EndValue - StartValue + 1;
  } else {
    mPerfCounterRange = StartValue - EndValue + 1;
  }

  //
  // Compute how many milliseconds the counter can measure before wrapping.
  // If mPerfCounterRange overflowed to 0, the counter is 64-bit and
  // effectively never wraps — set wrap time to MAX_UINT64.
  //
  if (mPerfCounterRange == 0) {
    mPerfCounterWrapMs = MAX_UINT64;
  } else {
    mPerfCounterWrapMs = mPerfCounterRange / mPerfCounterFreqKhz;
  }

  if (mPerfCounterWrapMs == MAX_UINT64) {
    Print (
      L"Performance counter: %lu KHz (%a, 64-bit, no wrap)\n",
      mPerfCounterFreqKhz,
      mPerfCounterCountsUp ? "counts up" : "counts down"
      );
  } else {
    Print (
      L"Performance counter: %lu KHz (%a, wraps every %lu ms)\n",
      mPerfCounterFreqKhz,
      mPerfCounterCountsUp ? "counts up" : "counts down",
      mPerfCounterWrapMs
      );
  }
}

static
UNIT_TEST_STATUS
EFIAPI
TimingPreReq (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  mTestIndex++;
  mTestStartCounter = GetPerformanceCounter ();
  gRT->GetTime (&mTestStartTime, NULL);

  //
  // Pre-register this test result slot. Pass/fail is synchronized from
  // the UnitTest framework result after RunAllTestSuites().
  //
  if (mTestResultCount < MAX_TEST_RESULTS) {
    mTestResults[mTestResultCount].TestNum   = mTestIndex;
    mTestResults[mTestResultCount].ElapsedMs = 0;
    mTestResults[mTestResultCount].Passed    = FALSE;
    mTestResultCount++;
  }

  return UNIT_TEST_PASSED;
}

static
VOID
EFIAPI
TimingCleanUp (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT64    EndCounter;
  UINT64    ElapsedMs;
  EFI_TIME  EndTime;

  EndCounter = GetPerformanceCounter ();
  ElapsedMs  = GetElapsedTicks (mTestStartCounter, EndCounter) / mPerfCounterFreqKhz;

  //
  // If the performance counter can wrap within 60s (e.g. 24-bit ACPI PM timer
  // wraps every ~4.7s), the single-wrap correction in GetElapsedTicks is only
  // valid for durations less than one wrap period.  For longer tests, fall back
  // to gRT->GetTime() which provides second-resolution wall-clock timing.
  //
  if ((mPerfCounterWrapMs < 60000) && (ElapsedMs > mPerfCounterWrapMs)) {
    gRT->GetTime (&EndTime, NULL);
    ElapsedMs = GetWallClockElapsedMs (&mTestStartTime, &EndTime);
  }

  Print (L"  [Timing] Test %02d: %lu ms\n", mTestIndex, ElapsedMs);

  //
  // Update the pre-registered entry with elapsed time.
  // The entry was created in TimingPreReq as the last element.
  //
  if ((mTestResultCount > 0) && (mTestResults[mTestResultCount - 1].TestNum == mTestIndex)) {
    mTestResults[mTestResultCount - 1].ElapsedMs = ElapsedMs;
  }
}

static
VOID
SyncSummaryResultsFromFramework (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework
  )
{
  APP_UNIT_TEST_FRAMEWORK  *Fw;
  LIST_ENTRY               *SuiteLink;

  Fw = (APP_UNIT_TEST_FRAMEWORK *)Framework;
  for (SuiteLink = Fw->TestSuiteList.ForwardLink; SuiteLink != &Fw->TestSuiteList; SuiteLink = SuiteLink->ForwardLink) {
    APP_UNIT_TEST_SUITE_LIST_ENTRY  *SuiteEntry;
    LIST_ENTRY                      *TestLink;

    SuiteEntry = BASE_CR (SuiteLink, APP_UNIT_TEST_SUITE_LIST_ENTRY, Entry);
    for (TestLink = SuiteEntry->UTS.TestCaseList.ForwardLink; TestLink != &SuiteEntry->UTS.TestCaseList; TestLink = TestLink->ForwardLink) {
      APP_UNIT_TEST_LIST_ENTRY  *TestEntry;
      UINTN                     TestNum;
      UINTN                     Idx;

      TestEntry = BASE_CR (TestLink, APP_UNIT_TEST_LIST_ENTRY, Entry);
      if ((TestEntry->UT.Name == NULL) || (AsciiStrnCmp (TestEntry->UT.Name, "Test", 4) != 0)) {
        continue;
      }

      TestNum = AsciiStrDecimalToUintn (TestEntry->UT.Name + 4);
      if ((TestNum == 0) || (TestNum > MAX_TEST_RESULTS)) {
        continue;
      }

      for (Idx = 0; Idx < mTestResultCount; Idx++) {
        if (mTestResults[Idx].TestNum == TestNum) {
          mTestResults[Idx].Passed = (BOOLEAN)(TestEntry->UT.Result == UNIT_TEST_PASSED);
          break;
        }
      }
    }
  }
}

// ============================================================================
// Test Framework Entry Point
// ============================================================================

static UINTN  mOnlyTest = 0;  // 0 = run all, non-zero = run only this test

static
BOOLEAN
ShouldRun (
  IN UINTN  TestNum
  )
{
  return (mOnlyTest == 0) || (mOnlyTest == TestNum);
}

static
EFI_STATUS
UefiTestMain (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      SpecConformanceSuite;
  UNIT_TEST_SUITE_HANDLE      FunctionalSuite;
  UINT64                      TimerPeriod;

  Framework = NULL;

  Print (L"DxeCoreTplTestApp: Starting...\n");
  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_NAME, UNIT_TEST_VERSION));

  //
  // Disable shell watchdog timer so tests can manage their own
  //
  gBS->SetWatchdogTimer (0, 0, 0, NULL);

  //
  // Locate required protocols
  //
  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&gCpu);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate EFI_CPU_ARCH_PROTOCOL: %r\n", Status));
    return Status;
  }

  Status = gBS->LocateProtocol (&gEfiTimerArchProtocolGuid, NULL, (VOID **)&gTimer);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate EFI_TIMER_ARCH_PROTOCOL: %r\n", Status));
    return Status;
  }

  //
  // Look up CoreTimerTick diagnostics from EFI System Configuration Table
  //
  {
    UINTN  Index;

    for (Index = 0; Index < gST->NumberOfTableEntries; Index++) {
      if (CompareGuid (&gST->ConfigurationTable[Index].VendorGuid, &gDxeCoreTimerTickDiagnosticsGuid)) {
        gTimerTickDiag = (CORE_TIMER_TICK_DIAGNOSTICS *)gST->ConfigurationTable[Index].VendorTable;
        if ((gTimerTickDiag != NULL) &&
            (gTimerTickDiag->Signature == CORE_TIMER_TICK_DIAGNOSTICS_SIGNATURE))
        {
          Print (
            L"  Timer diagnostics: TotalEntries=%u MaxDepth=%u\n",
            (UINT32)gTimerTickDiag->TotalEntries,
            (UINT32)gTimerTickDiag->MaxDepth
            );
        } else {
          Print (L"  WARNING: Timer diagnostics table invalid\n");
          gTimerTickDiag = NULL;
        }

        break;
      }
    }

    if (gTimerTickDiag == NULL) {
      Print (L"  ERROR: Timer diagnostics config table not found (requires DEBUG_CODE enabled via PcdDebugPropertyMask)\n");
      return EFI_NOT_FOUND;
    }
  }

  //
  // Determine timer period in microseconds
  //
  Status = gTimer->GetTimerPeriod (gTimer, &TimerPeriod);
  if (EFI_ERROR (Status) || (TimerPeriod == 0)) {
    DEBUG ((DEBUG_ERROR, "Failed to get timer period or timer not running: %r\n", Status));
    return EFI_DEVICE_ERROR;
  }

  mTimerPeriodUs = (UINTN)(TimerPeriod / 10);
  if (mTimerPeriodUs == 0) {
    mTimerPeriodUs = 1;
  }

  DEBUG ((DEBUG_INFO, "Timer period: %lu (100ns units) = %u us\n", TimerPeriod, mTimerPeriodUs));

  //
  // Calibrate performance counter for per-test timing
  //
  CalibratePerformanceCounter ();

  //
  // Initialize the test framework
  //
  Status = InitUnitTestFramework (&Framework, UNIT_TEST_NAME, gEfiCallerBaseName, UNIT_TEST_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  // --------------------------------------------------------------------------
  // Suite 1: Spec Conformance (Tests 1-7)
  // --------------------------------------------------------------------------
  Status = CreateUnitTestSuite (
             &SpecConformanceSuite,
             Framework,
             "PI/UEFI Spec Conformance",
             "DxeCoreTpl.SpecConformance",
             NULL,
             NULL
             );
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  if (ShouldRun (1)) {
    AddTestCase (SpecConformanceSuite, "RaiseTpl(HIGH)/RestoreTpl round-trip", "Test01", Test01RaiseTplHighRestoreTpl, TimingPreReq, TimingCleanUp, NULL);
  }

  if (ShouldRun (2)) {
    AddTestCase (SpecConformanceSuite, "RaiseTpl to non-HIGH", "Test02", Test02RaiseTplNonHigh, TimingPreReq, TimingCleanUp, NULL);
  }

  if (ShouldRun (3)) {
    AddTestCase (SpecConformanceSuite, "Nested RaiseTpl(HIGH) at HIGH", "Test03", Test03NestedRaiseTplAtHigh, TimingPreReq, TimingCleanUp, NULL);
  }

  if (ShouldRun (4)) {
    AddTestCase (SpecConformanceSuite, "Event dispatch with interrupts enabled", "Test04", Test04EventDispatchInterrupts, TimingPreReq, TimingCleanUp, NULL);
  }

  if (ShouldRun (5)) {
    AddTestCase (SpecConformanceSuite, "DisableIRQ then RaiseTpl(HIGH)", "Test05", Test05DisableIrqThenRaiseTpl, TimingPreReq, TimingCleanUp, NULL);
  }

  if (ShouldRun (6)) {
    AddTestCase (SpecConformanceSuite, "DisableIRQ at TPL_HIGH (no-op)", "Test06", Test06DisableIrqAtHigh, TimingPreReq, TimingCleanUp, NULL);
  }

  if (ShouldRun (7)) {
    AddTestCase (SpecConformanceSuite, "Toggle IRQ without TPL change", "Test07", Test07ToggleIrqNoTpl, TimingPreReq, TimingCleanUp, NULL);
  }

  // --------------------------------------------------------------------------
  // Suite 2: Functional (Tests 8-16)
  // --------------------------------------------------------------------------
  Status = CreateUnitTestSuite (
             &FunctionalSuite,
             Framework,
             "Recursion Fix Functional Tests",
             "DxeCoreTpl.Functional",
             NULL,
             NULL
             );
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  if (ShouldRun (8)) {
    AddTestCase (FunctionalSuite, "Non-monotonic TPL (HIGH->APP->HIGH)", "Test08", Test08NonMonotonicTpl, TimingPreReq, TimingCleanUp, NULL);
  }

  if (ShouldRun (9)) {
    AddTestCase (FunctionalSuite, "Timer IRQ during event dispatch", "Test09", Test09TimerIrqDuringDispatch, TimingPreReq, TimingCleanUp, NULL);
  }

  if (ShouldRun (10)) {
    AddTestCase (FunctionalSuite, "EnableIRQ at TPL_HIGH (misuse)", "Test10", Test10EnableIrqAtHigh, TimingPreReq, TimingCleanUp, NULL);
  }

  if (ShouldRun (11)) {
    AddTestCase (FunctionalSuite, "Sustained RaiseTpl/RestoreTpl (100K iterations)", "Test11", Test11SustainedStress, TimingPreReq, TimingCleanUp, NULL);
  }

  if (ShouldRun (12)) {
    AddTestCase (FunctionalSuite, "Rapid TPL cycling across all levels", "Test12", Test12RapidTplCycling, TimingPreReq, TimingCleanUp, NULL);
  }

  if (ShouldRun (13)) {
    AddTestCase (FunctionalSuite, "Timer event signaling during TPL cycling", "Test13", Test13TimerEventVerify, TimingPreReq, TimingCleanUp, NULL);
  }

  if (ShouldRun (14)) {
    AddTestCase (FunctionalSuite, "Non-HIGH RestoreTpl after timer IRQ (regression)", "Test14", Test14NonHighRestoreTplRegression, TimingPreReq, TimingCleanUp, NULL);
  }

  if (ShouldRun (15)) {
    AddTestCase (FunctionalSuite, "Forced timer recursion (bounded depth)", "Test15", Test15ForcedTimerRecursion, TimingPreReq, TimingCleanUp, NULL);
  }

  if (ShouldRun (16)) {
    AddTestCase (FunctionalSuite, "Natural timer recursion (bounded depth)", "Test16", Test16NaturalTimerRecursion, TimingPreReq, TimingCleanUp, NULL);
  }

  // --------------------------------------------------------------------------
  // Execute all test suites
  // --------------------------------------------------------------------------
  mTotalStartCounter = GetPerformanceCounter ();
  gRT->GetTime (&mTotalStartTime, NULL);
  Status = RunAllTestSuites (Framework);
  SyncSummaryResultsFromFramework (Framework);
  {
    UINT64    TotalEndCounter;
    UINT64    TotalElapsedMs;
    UINT64    TotalElapsedFromTestsMs;
    EFI_TIME  TotalEndTime;

    TotalEndCounter = GetPerformanceCounter ();
    TotalElapsedMs  = GetElapsedTicks (mTotalStartCounter, TotalEndCounter) / mPerfCounterFreqKhz;

    //
    // Fall back to wall-clock for total time on narrow counters
    //
    if ((mPerfCounterWrapMs < 60000) && (TotalElapsedMs > mPerfCounterWrapMs)) {
      gRT->GetTime (&TotalEndTime, NULL);
      TotalElapsedMs = GetWallClockElapsedMs (&mTotalStartTime, &TotalEndTime);
    }

    //
    // Prefer aggregate time derived from per-test measurements.
    // This avoids long-interval perf-counter anomalies seen on some platforms.
    //
    TotalElapsedFromTestsMs = 0;

    //
    // Print summary table with pass/fail and timing for each test
    //
    Print (L"\n=========================================================\n");
    Print (L"  TEST SUMMARY\n");
    Print (L"---------------------------------------------------------\n");
    Print (L"  Test  | Result | Time (ms)\n");
    Print (L"  ------+--------+----------\n");
    {
      UINTN  Idx;
      UINTN  PassCount;
      UINTN  FailCount;

      PassCount = 0;
      FailCount = 0;
      for (Idx = 0; Idx < mTestResultCount; Idx++) {
        Print (
          L"    %02d  | %4a |  %lu\n",
          mTestResults[Idx].TestNum,
          mTestResults[Idx].Passed ? "PASS" : "FAIL",
          mTestResults[Idx].ElapsedMs
          );
        TotalElapsedFromTestsMs += mTestResults[Idx].ElapsedMs;
        if (mTestResults[Idx].Passed) {
          PassCount++;
        } else {
          FailCount++;
        }
      }

      Print (L"---------------------------------------------------------\n");
      Print (
        L"  Total: %u tests | Passed: %u | Failed: %u\n",
        (UINT32)mTestResultCount,
        (UINT32)PassCount,
        (UINT32)FailCount
        );
    }

    if (mTestResultCount > 0) {
      TotalElapsedMs = TotalElapsedFromTestsMs;
    }

    Print (
      L"  Total test execution time: %lu ms (%lu.%03lu s)\n",
      TotalElapsedMs,
      TotalElapsedMs / 1000,
      TotalElapsedMs % 1000
      );
    Print (L"=========================================================\n");
  }

EXIT:
  if (Framework) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}

/**
  Standard UEFI entry point for UEFI Shell execution.
  Optional argument: test number to run in isolation (e.g. "DxeCoreTplTestApp.efi 4")
  If argument matches an "expected to fail" test, only that test is registered.
  If no argument or 0, all tests are registered.
**/
EFI_STATUS
EFIAPI
DxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                 Status;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  CHAR16                     *Options;
  UINTN                      TestNum;

  //
  // Parse optional command-line argument for single-test mode
  //
  Status = gBS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&LoadedImage);
  if (!EFI_ERROR (Status) && (LoadedImage->LoadOptions != NULL) && (LoadedImage->LoadOptionsSize > 0)) {
    Options = (CHAR16 *)LoadedImage->LoadOptions;

    //
    // Skip any leading spaces
    //
    while (*Options == L' ') {
      Options++;
    }

    //
    // Parse a decimal number
    //
    TestNum = 0;
    while ((*Options >= L'0') && (*Options <= L'9')) {
      TestNum = TestNum * 10 + (*Options - L'0');
      Options++;
    }

    if (TestNum != 0) {
      mOnlyTest = TestNum;
      Print (L"DxeCoreTplTestApp: Single-test mode - running only Test %u\n", mOnlyTest);
    }
  }

  return UefiTestMain ();
}
