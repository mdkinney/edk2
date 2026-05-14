/** @file
  Shared header for the DXE Core TPL Timer Interrupt Recursion Test.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef DXE_CORE_TPL_TEST_H_
#define DXE_CORE_TPL_TEST_H_

#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseLib.h>
#include <Library/TimerLib.h>
#include <Protocol/Cpu.h>
#include <Protocol/Timer.h>

//
// Protocol instances (defined in DxeCoreTplTestApp.c)
//
extern EFI_CPU_ARCH_PROTOCOL    *gCpu;
extern EFI_TIMER_ARCH_PROTOCOL  *gTimer;

//
// Timer period in microseconds (defined in DxeCoreTplTestApp.c)
//
extern UINTN  mTimerPeriodUs;

//
// Shared volatile state for IRQ-context tests (defined in DxeCoreTplTestApp.c)
//
extern volatile BOOLEAN  mHandlerExecuted;
extern volatile BOOLEAN  mHandlerPassed;
extern volatile UINTN    mHandlerIterations;
extern volatile BOOLEAN  mInterruptStateInHandler;
extern volatile BOOLEAN  mEventsDispatched;
extern volatile BOOLEAN  mNotifyCallbackFired;
extern volatile BOOLEAN  mNotifyInterruptState;
extern volatile UINTN    mNotifyDispatchCount;
extern volatile BOOLEAN  mCallbackInterruptState;
extern volatile UINTN    mCallbackDispatchCount;
extern volatile BOOLEAN  mLockCallbackExecuted;
extern volatile BOOLEAN  mCallbackSurvived;

//
// TPL preemption hierarchy test state (Test 26)
//
extern volatile UINTN    mPreemptNotifyCountAtStart;
extern volatile UINTN    mPreemptNotifyCountAtEnd;
extern volatile BOOLEAN  mPreemptCallbackStarted;
extern volatile BOOLEAN  mPreemptCallbackFinished;

//
// Events used by IRQ-context tests (defined in DxeCoreTplTestApp.c)
//
extern EFI_EVENT  mTestNotifyEvent;
extern EFI_EVENT  mTestCallbackEvent;

// ============================================================================
// Helper function prototypes (defined in DxeCoreTplTestApp.c)
// ============================================================================

BOOLEAN
GetInterruptStateChecked (
  VOID
  );

UINTN
StallForTicks (
  IN UINTN  TickCount
  );

VOID
SetRecursionWatchdog (
  IN UINTN  TimeoutSeconds
  );

/**
  Check if we are resuming after a watchdog-triggered reset.
  If Context is non-NULL, the framework restored saved state from a prior
  crash (the test was in RUNNING state when the system reset).

  @param[in]  Context   The context passed to the test function.

  @retval TRUE   This is a resume after a crash - test should fail immediately.
  @retval FALSE  Normal first execution.
**/
BOOLEAN
IsResumeAfterWatchdogReset (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Save framework state with a sentinel marker before executing a dangerous
  code section.  If the system resets (watchdog fires), on resume the
  framework will restore this context and IsResumeAfterWatchdogReset()
  will return TRUE.
**/
VOID
SaveStateBeforeDangerousTest (
  VOID
  );

VOID
ClearRecursionWatchdog (
  VOID
  );

VOID
ResetHandlerState (
  VOID
  );

EFI_STATUS
InstallTestTimerHandler (
  IN EFI_TIMER_NOTIFY  Handler
  );

EFI_STATUS
UninstallTestTimerHandler (
  VOID
  );

// ============================================================================
// Event callbacks shared across test files (defined in DxeCoreTplTestApp.c)
// ============================================================================

VOID
EFIAPI
NotifyRecordCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

VOID
EFIAPI
CallbackRecordCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

VOID
EFIAPI
SlowNotifyCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

VOID
EFIAPI
LockingCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

VOID
EFIAPI
PreemptNotifyCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

VOID
EFIAPI
PreemptSlowCallbackHandler (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

// ============================================================================
// Test function prototypes - Suite 1: TPL-Managed (TplManagedTests.c)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test01NormalRaiseTplRestoreTpl (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test02RaiseTplNonHigh (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test03NestedRaiseTplAtHigh (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test04NoRecursionUnderTimer (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test05NormalContextAfterTimerIrq (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test06TempLowerAcrossHigh (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test07NormalTplNesting (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test08TimerIrqDuringDispatch (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test26TplPreemptionHierarchy (
  IN UNIT_TEST_CONTEXT  Context
  );

// ============================================================================
// Test function prototypes - Suite 2: CPU Protocol (CpuProtocolTests.c)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test09DisableIrqThenRaiseTpl (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test10DisableIrqAtHigh (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test11ToggleIrqNoTpl (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test12EnableIrqAtHigh (
  IN UNIT_TEST_CONTEXT  Context
  );

// ============================================================================
// Test function prototypes - Suite 3: Arch-Specific (ArchSpecificTests.c)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test13CliThenRaiseTpl (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test14CliStiBracketNoTpl (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test15StiAtHigh (
  IN UNIT_TEST_CONTEXT  Context
  );

// ============================================================================
// Test function prototypes - Suite 4: Stress (StressTests.c)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test20SustainedStress (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test21RapidTplCycling (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test22TimerEventVerify (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test23NonHighRestoreTplRegression (
  IN UNIT_TEST_CONTEXT  Context
  );

// ============================================================================
// Test function prototypes - Suite 5: IRQ Hook (IrqHookTests.c)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test16IrqContextRaiseTplRestoreTpl (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test17IrqContextEventDispatch (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test18IrqContextSustainedStress (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test19IrqContextTempLowerTpl (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test24IrqContextCallbackRaiseTpl (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test25IrqContextIntermediateTpl (
  IN UNIT_TEST_CONTEXT  Context
  );

#endif // DXE_CORE_TPL_TEST_H_
