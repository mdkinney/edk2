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
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TimerLib.h>
#include <Protocol/Cpu.h>
#include <Protocol/Timer.h>
#include <Protocol/LoadedImage.h>
#include <Guid/DxeCoreDiagnostics.h>

//
// Protocol instances (defined in DxeCoreTplTestApp.c)
//
extern EFI_CPU_ARCH_PROTOCOL    *gCpu;
extern EFI_TIMER_ARCH_PROTOCOL  *gTimer;

//
// Pointer to DxeCore's timer tick diagnostics (looked up from config table)
//
extern CORE_TIMER_TICK_DIAGNOSTICS  *gTimerTickDiag;

//
// Timer period in microseconds (defined in DxeCoreTplTestApp.c)
//
extern UINTN  mTimerPeriodUs;

//
// Performance counter timing infrastructure (defined in DxeCoreTplTestApp.c)
//
extern UINT64   mPerfCounterFreqKhz;
extern BOOLEAN  mPerfCounterCountsUp;

UINT64
GetElapsedTicks (
  IN UINT64  Start,
  IN UINT64  End
  );

//
// Shared volatile state for event callbacks (defined in DxeCoreTplTestApp.c)
//
extern volatile BOOLEAN  mNotifyCallbackFired;
extern volatile BOOLEAN  mNotifyInterruptState;
extern volatile UINTN    mNotifyDispatchCount;
extern volatile BOOLEAN  mCallbackInterruptState;
extern volatile UINTN    mCallbackDispatchCount;
extern volatile BOOLEAN  mCallbackSurvived;

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

// ============================================================================
// Test function prototypes - Suite 1: Spec Conformance (SpecConformanceTests.c)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test01RaiseTplHighRestoreTpl (
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
Test04EventDispatchInterrupts (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test05DisableIrqThenRaiseTpl (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test06DisableIrqAtHigh (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test07ToggleIrqNoTpl (
  IN UNIT_TEST_CONTEXT  Context
  );

// ============================================================================
// Test function prototypes - Suite 2: Functional (FunctionalTests.c)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test08NonMonotonicTpl (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test09TimerIrqDuringDispatch (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test10EnableIrqAtHigh (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test11SustainedStress (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test12RapidTplCycling (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test13TimerEventVerify (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test14NonHighRestoreTplRegression (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test15ForcedTimerRecursion (
  IN UNIT_TEST_CONTEXT  Context
  );

UNIT_TEST_STATUS
EFIAPI
Test16NaturalTimerRecursion (
  IN UNIT_TEST_CONTEXT  Context
  );

#endif // DXE_CORE_TPL_TEST_H_
