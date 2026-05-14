/** @file
  DXE Core TPL Timer Interrupt Recursion Test Application - Main Runner.

  This UEFI Shell application verifies that the mIsrEntryTplMask-based fix
  in CoreRaiseTpl/CoreRestoreTpl (MdeModulePkg/Core/Dxe/Event/Tpl.c) prevents
  infinite recursion from timer interrupts while preserving normal TPL and
  interrupt behavior.

  The fix adds a bitmask (mIsrEntryTplMask) that tracks which TPL levels
  were interrupted by a timer IRQ.  CoreRaiseTpl sets a bit when it detects
  interrupts are already disabled (indicating IRQ context), and CoreRestoreTpl
  uses the mask to decide whether to re-enable interrupts or leave them
  disabled for the hardware IRET to restore.

  This file contains:
  - Global variable definitions shared across all test source files
  - Helper functions (watchdog, interrupt state, timer handler install)
  - Event callback functions used by multiple test suites
  - Performance counter timing infrastructure (portable across IA32/X64/AARCH64)
  - The main test framework entry point that registers all 26 test cases
    organized into 5 suites and measures total execution time

  Tests cover all 25 scenarios from the TimerInterruptRecursionAnalysis.md:
  - Tests 1-8:   TPL-managed scenarios (normal operation, nesting, events)
  - Tests 9-12:  CPU protocol interrupt manipulation (DisableInterrupt/EnableInterrupt)
  - Tests 13-15: Architecture-specific instruction scenarios (CLI/STI equivalents)
  - Tests 16-19: IRQ-context tests via timer handler hook (direct interrupt context)
  - Tests 20-23: Stress and stability (100K iterations, rapid cycling, timer events)
  - Tests 24-25: Advanced IRQ-context (callback nesting, intermediate TPL restore)
  - Test 26:     TPL preemption hierarchy (bounded preemption validation)

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "DxeCoreTplTest.h"

#define UNIT_TEST_NAME     "DXE Core TPL Timer Interrupt Recursion Test"
#define UNIT_TEST_VERSION  "1.0"

//
// Protocol instances
//
EFI_CPU_ARCH_PROTOCOL    *gCpu   = NULL;
EFI_TIMER_ARCH_PROTOCOL  *gTimer = NULL;

//
// Timer period in microseconds (computed at init)
//
UINTN  mTimerPeriodUs = 0;

//
// Shared volatile state for IRQ-context tests (Tests 16-19, 24-25)
//
volatile BOOLEAN  mHandlerExecuted          = FALSE;
volatile BOOLEAN  mHandlerPassed            = FALSE;
volatile UINTN    mHandlerIterations        = 0;
volatile BOOLEAN  mInterruptStateInHandler  = FALSE;
volatile BOOLEAN  mEventsDispatched         = FALSE;
volatile BOOLEAN  mNotifyCallbackFired      = FALSE;
volatile BOOLEAN  mNotifyInterruptState     = FALSE;
volatile UINTN    mNotifyDispatchCount      = 0;
volatile BOOLEAN  mCallbackInterruptState   = FALSE;
volatile UINTN    mCallbackDispatchCount    = 0;
volatile BOOLEAN  mLockCallbackExecuted     = FALSE;
volatile BOOLEAN  mCallbackSurvived         = FALSE;

//
// Shared volatile state for TPL preemption hierarchy test (Test 26)
//
volatile UINTN    mPreemptNotifyCountAtStart = 0;
volatile UINTN    mPreemptNotifyCountAtEnd   = 0;
volatile BOOLEAN  mPreemptCallbackStarted    = FALSE;
volatile BOOLEAN  mPreemptCallbackFinished   = FALSE;

//
// Events used by IRQ-context tests
//
EFI_EVENT  mTestNotifyEvent   = NULL;
EFI_EVENT  mTestCallbackEvent = NULL;

//
// Timing infrastructure
//
static UINT64   mPerfCounterFreqKhz  = 0;
static BOOLEAN  mPerfCounterCountsUp = TRUE;
static UINT64   mTestStartCounter    = 0;
static UINT64   mTotalStartCounter   = 0;
static UINTN    mTestIndex           = 0;

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

VOID
ResetHandlerState (
  VOID
  )
{
  mHandlerExecuted         = FALSE;
  mHandlerPassed           = FALSE;
  mHandlerIterations       = 0;
  mInterruptStateInHandler = FALSE;
  mEventsDispatched        = FALSE;
  mNotifyCallbackFired     = FALSE;
  mNotifyInterruptState    = FALSE;
  mNotifyDispatchCount     = 0;
  mCallbackInterruptState  = FALSE;
  mCallbackDispatchCount   = 0;
  mLockCallbackExecuted    = FALSE;
  mCallbackSurvived        = FALSE;
}

EFI_STATUS
InstallTestTimerHandler (
  IN EFI_TIMER_NOTIFY  Handler
  )
{
  EFI_STATUS  Status;

  //
  // Unregister any existing handler (ignore error if none registered)
  //
  Status = gTimer->RegisterHandler (gTimer, NULL);
  if (EFI_ERROR (Status) && (Status != EFI_INVALID_PARAMETER)) {
    return Status;
  }

  //
  // Register test handler
  //
  return gTimer->RegisterHandler (gTimer, Handler);
}

EFI_STATUS
UninstallTestTimerHandler (
  VOID
  )
{
  return gTimer->RegisterHandler (gTimer, NULL);
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

VOID
EFIAPI
LockingCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_TPL  LockTpl;

  //
  // Simulate lock acquire/release (common pattern in real drivers)
  //
  LockTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->RestoreTPL (LockTpl);
  mLockCallbackExecuted = TRUE;
}

VOID
EFIAPI
PreemptNotifyCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  //
  // This is a periodic timer event at TPL_NOTIFY.
  // Just increment the dispatch count.  The preemption test checks
  // whether this count increases while the CALLBACK handler stalls.
  //
  mNotifyDispatchCount++;
}

VOID
EFIAPI
PreemptSlowCallbackHandler (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  //
  // Record NOTIFY dispatch count at the start of this handler.
  //
  mPreemptCallbackStarted    = TRUE;
  mPreemptNotifyCountAtStart = mNotifyDispatchCount;

  //
  // Stall for several timer periods.  With bounded preemption enabled,
  // a timer interrupt will fire during this stall, preempt us, and
  // dispatch the periodic NOTIFY event (which increments mNotifyDispatchCount).
  //
  gBS->Stall (StallForTicks (5));

  //
  // Record NOTIFY dispatch count at the end.  If preemption worked,
  // this will be greater than the start count.
  //
  mPreemptNotifyCountAtEnd  = mNotifyDispatchCount;
  mPreemptCallbackFinished  = TRUE;
}

// ============================================================================
// Timing Infrastructure
// ============================================================================

static
UINT64
GetElapsedTicks (
  IN UINT64  Start,
  IN UINT64  End
  )
{
  if (mPerfCounterCountsUp) {
    return End - Start;
  } else {
    return Start - End;
  }
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

  Freq = GetPerformanceCounterProperties (&StartValue, &EndValue);
  mPerfCounterCountsUp = (StartValue < EndValue);
  mPerfCounterFreqKhz  = Freq / 1000;  // Hz to KHz (ticks per ms)
  if (mPerfCounterFreqKhz == 0) {
    mPerfCounterFreqKhz = 1;
  }

  Print (
    L"Performance counter: %lu KHz (%a)\n",
    mPerfCounterFreqKhz,
    mPerfCounterCountsUp ? "counts up" : "counts down"
    );
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
  return UNIT_TEST_PASSED;
}

static
VOID
EFIAPI
TimingCleanUp (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT64  EndCounter;
  UINT64  ElapsedMs;

  EndCounter = GetPerformanceCounter ();
  ElapsedMs  = GetElapsedTicks (mTestStartCounter, EndCounter) / mPerfCounterFreqKhz;

  Print (L"  [Timing] Test %02d: %lu ms\n", mTestIndex, ElapsedMs);
}

// ============================================================================
// Test Framework Entry Point
// ============================================================================

static
EFI_STATUS
UefiTestMain (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      TplManagedSuite;
  UNIT_TEST_SUITE_HANDLE      CpuProtocolSuite;
  UNIT_TEST_SUITE_HANDLE      ArchSpecificSuite;
  UNIT_TEST_SUITE_HANDLE      IrqHookSuite;
  UNIT_TEST_SUITE_HANDLE      StressSuite;
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
  // Suite 1: TPL-Managed Scenarios (Tests 1-8)
  // --------------------------------------------------------------------------
  Status = CreateUnitTestSuite (
             &TplManagedSuite,
             Framework,
             "TPL-Managed Interrupt Scenarios",
             "DxeCoreTpl.TplManaged",
             NULL,
             NULL
             );
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  AddTestCase (TplManagedSuite, "Normal RaiseTpl(HIGH)/RestoreTpl", "Test01", Test01NormalRaiseTplRestoreTpl, TimingPreReq, TimingCleanUp, NULL);
  AddTestCase (TplManagedSuite, "RaiseTpl to non-HIGH", "Test02", Test02RaiseTplNonHigh, TimingPreReq, TimingCleanUp, NULL);
  AddTestCase (TplManagedSuite, "Nested RaiseTpl(HIGH) at HIGH", "Test03", Test03NestedRaiseTplAtHigh, TimingPreReq, TimingCleanUp, NULL);
  AddTestCase (TplManagedSuite, "No recursion under timer load", "Test04", Test04NoRecursionUnderTimer, TimingPreReq, TimingCleanUp, NULL);
  AddTestCase (TplManagedSuite, "Normal context after timer IRQ", "Test05", Test05NormalContextAfterTimerIrq, TimingPreReq, TimingCleanUp, NULL);
  AddTestCase (TplManagedSuite, "Temp lower TPL across HIGH boundary", "Test06", Test06TempLowerAcrossHigh, TimingPreReq, TimingCleanUp, NULL);
  AddTestCase (TplManagedSuite, "Normal TPL nesting via event dispatch", "Test07", Test07NormalTplNesting, TimingPreReq, TimingCleanUp, NULL);
  AddTestCase (TplManagedSuite, "Timer IRQ during normal event dispatch", "Test08", Test08TimerIrqDuringDispatch, TimingPreReq, TimingCleanUp, NULL);
  AddTestCase (TplManagedSuite, "TPL preemption: CALLBACK preempted by NOTIFY", "Test26", Test26TplPreemptionHierarchy, TimingPreReq, TimingCleanUp, NULL);

  // --------------------------------------------------------------------------
  // Suite 2: CPU Protocol Manipulation Scenarios (Tests 9-12)
  // --------------------------------------------------------------------------
  Status = CreateUnitTestSuite (
             &CpuProtocolSuite,
             Framework,
             "CPU Protocol Interrupt Manipulation",
             "DxeCoreTpl.CpuProtocol",
             NULL,
             NULL
             );
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  AddTestCase (CpuProtocolSuite, "DisableIRQ then RaiseTpl(HIGH)", "Test09", Test09DisableIrqThenRaiseTpl, TimingPreReq, TimingCleanUp, NULL);
  AddTestCase (CpuProtocolSuite, "DisableIRQ at TPL_HIGH", "Test10", Test10DisableIrqAtHigh, TimingPreReq, TimingCleanUp, NULL);
  AddTestCase (CpuProtocolSuite, "Toggle IRQ without TPL change", "Test11", Test11ToggleIrqNoTpl, TimingPreReq, TimingCleanUp, NULL);
  AddTestCase (CpuProtocolSuite, "EnableIRQ at TPL_HIGH (misuse)", "Test12", Test12EnableIrqAtHigh, TimingPreReq, TimingCleanUp, NULL);

  // --------------------------------------------------------------------------
  // Suite 3: Architecture-Specific Instruction Scenarios (Tests 13-15)
  // --------------------------------------------------------------------------
  Status = CreateUnitTestSuite (
             &ArchSpecificSuite,
             Framework,
             "Architecture-Specific Interrupt Instructions",
             "DxeCoreTpl.ArchSpecific",
             NULL,
             NULL
             );
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  AddTestCase (ArchSpecificSuite, "CLI then RaiseTpl(HIGH)", "Test13", Test13CliThenRaiseTpl, TimingPreReq, TimingCleanUp, NULL);
  AddTestCase (ArchSpecificSuite, "CLI/STI bracket without TPL change", "Test14", Test14CliStiBracketNoTpl, TimingPreReq, TimingCleanUp, NULL);
  AddTestCase (ArchSpecificSuite, "STI at TPL_HIGH (misuse)", "Test15", Test15StiAtHigh, TimingPreReq, TimingCleanUp, NULL);

  // --------------------------------------------------------------------------
  // Suite 4: Stress and Stability (Tests 20-23)
  // --------------------------------------------------------------------------
  Status = CreateUnitTestSuite (
             &StressSuite,
             Framework,
             "Stress and Stability Tests",
             "DxeCoreTpl.Stress",
             NULL,
             NULL
             );
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  AddTestCase (StressSuite, "Sustained RaiseTpl/RestoreTpl (100K iterations)", "Test20", Test20SustainedStress, TimingPreReq, TimingCleanUp, NULL);
  AddTestCase (StressSuite, "Rapid TPL cycling across all levels", "Test21", Test21RapidTplCycling, TimingPreReq, TimingCleanUp, NULL);
  AddTestCase (StressSuite, "Timer event signaling during TPL cycling", "Test22", Test22TimerEventVerify, TimingPreReq, TimingCleanUp, NULL);
  AddTestCase (StressSuite, "Non-HIGH RestoreTpl after timer IRQ (regression)", "Test23", Test23NonHighRestoreTplRegression, TimingPreReq, TimingCleanUp, NULL);

  // --------------------------------------------------------------------------
  // Suite 5: IRQ Hook Tests (Tests 16-19, 24-25)
  // These MUST run LAST - they replace CoreTimerTick permanently
  // --------------------------------------------------------------------------
  Status = CreateUnitTestSuite (
             &IrqHookSuite,
             Framework,
             "IRQ-Context Tests via Timer Handler Hook",
             "DxeCoreTpl.IrqHook",
             NULL,
             NULL
             );
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  AddTestCase (IrqHookSuite, "IRQ-context RaiseTpl/RestoreTpl", "Test16", Test16IrqContextRaiseTplRestoreTpl, TimingPreReq, TimingCleanUp, NULL);
  AddTestCase (IrqHookSuite, "IRQ-context event dispatch", "Test17", Test17IrqContextEventDispatch, TimingPreReq, TimingCleanUp, NULL);
  AddTestCase (IrqHookSuite, "IRQ-context sustained stress", "Test18", Test18IrqContextSustainedStress, TimingPreReq, TimingCleanUp, NULL);
  AddTestCase (IrqHookSuite, "IRQ-context temp lower TPL", "Test19", Test19IrqContextTempLowerTpl, TimingPreReq, TimingCleanUp, NULL);
  AddTestCase (IrqHookSuite, "IRQ-context callback RaiseTpl(HIGH)", "Test24", Test24IrqContextCallbackRaiseTpl, TimingPreReq, TimingCleanUp, NULL);
  AddTestCase (IrqHookSuite, "IRQ-context intermediate TPL restore", "Test25", Test25IrqContextIntermediateTpl, TimingPreReq, TimingCleanUp, NULL);

  // --------------------------------------------------------------------------
  // Execute all test suites
  // --------------------------------------------------------------------------
  mTotalStartCounter = GetPerformanceCounter ();
  Status = RunAllTestSuites (Framework);
  {
    UINT64  TotalEndCounter;
    UINT64  TotalElapsedMs;

    TotalEndCounter = GetPerformanceCounter ();
    TotalElapsedMs  = GetElapsedTicks (mTotalStartCounter, TotalEndCounter) / mPerfCounterFreqKhz;
    Print (L"\n=========================================================\n");
    Print (L"  Total test execution time: %lu ms (%lu.%03lu s)\n",
      TotalElapsedMs, TotalElapsedMs / 1000, TotalElapsedMs % 1000);
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
**/
EFI_STATUS
EFIAPI
DxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return UefiTestMain ();
}
