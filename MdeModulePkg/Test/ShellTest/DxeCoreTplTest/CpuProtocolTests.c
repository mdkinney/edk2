/** @file
  CPU Protocol Interrupt Manipulation Tests (Tests 9-12).

  These tests verify correct behavior when drivers use the EFI_CPU_ARCH_PROTOCOL
  DisableInterrupt/EnableInterrupt APIs in combination with TPL operations.
  These represent real-world misuse patterns that can confuse the interrupt
  state tracking in the mIsrEntryTplMask fix.

  Test 09: DisableInterrupt then RaiseTpl(HIGH).
           With interrupts already off, RaiseTpl sees GetInterruptState=FALSE
           and sets a mask bit (false positive).  Verifies the system handles
           this gracefully - interrupts remain OFF after RestoreTpl (known
           acceptable behavior), and manual EnableInterrupt restores normalcy.

  Test 10: DisableInterrupt while at TPL_HIGH_LEVEL.
           Calling DisableInterrupt when already at HIGH is redundant but
           must not corrupt state.  Verifies RestoreTpl still re-enables
           interrupts correctly.

  Test 11: Toggle interrupts without any TPL change.
           DisableInterrupt/EnableInterrupt without RaiseTpl/RestoreTpl.
           Verifies the mask is not affected by interrupt toggling alone.

  Test 12: EnableInterrupt at TPL_HIGH_LEVEL (misuse scenario).
           Enabling interrupts while at HIGH allows timer interrupts to fire
           at HIGH - the exact scenario the fix prevents from recursing.
           Uses a watchdog to detect hang.  Verifies system survival.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "DxeCoreTplTest.h"

// ============================================================================
// Test 9 - Disable interrupts then RaiseTpl(HIGH) (Scenarios 10, 15, 18)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test09DisableIrqThenRaiseTpl (
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
  // False positive: interrupts remain OFF (known behavior for all solutions)
  //
  UT_ASSERT_FALSE (GetInterruptStateChecked ());

  //
  // Manually restore, then verify self-clean
  //
  gCpu->EnableInterrupt (gCpu);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->RestoreTPL (OldTpl);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  return UNIT_TEST_PASSED;
}

// ============================================================================
// Test 10 - Disable interrupts in interrupt context (Scenario 12)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test10DisableIrqAtHigh (
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
// Test 11 - Toggle interrupts without TPL change (Scenarios 13, 16, 17)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test11ToggleIrqNoTpl (
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

// ============================================================================
// Test 12 - Enable interrupts at TPL_HIGH (Scenario 19)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test12EnableIrqAtHigh (
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
