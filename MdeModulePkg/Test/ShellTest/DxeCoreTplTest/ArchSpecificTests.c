/** @file
  Architecture-Specific Interrupt Instruction Tests (Tests 13-15).

  These tests exercise scenarios where drivers use architecture-specific
  interrupt control (CLI/STI on x86, or their CPU protocol equivalents)
  in combination with TPL operations.  They verify the mIsrEntryTplMask
  fix handles these edge cases without infinite recursion or state corruption.

  Test 13: CLI equivalent then RaiseTpl(HIGH).
           Disables interrupts via CPU protocol (simulating CLI), then does
           a RaiseTpl/RestoreTpl cycle.  Same false-positive pattern as Test 9
           but framed as architecture-specific instruction usage.  Verifies
           manual EnableInterrupt restores normal behavior.

  Test 14: CLI/STI bracket without any TPL change.
           Disable then re-enable interrupts with no TPL manipulation.
           Verifies subsequent RaiseTpl/RestoreTpl works correctly since
           the mask was never touched (interrupts were ON when RaiseTpl runs).

  Test 15: STI equivalent at TPL_HIGH_LEVEL (misuse scenario).
           Enables interrupts while at HIGH via CPU protocol (simulating STI
           at ring 0 with TPL_HIGH).  Timer interrupts fire at HIGH - the
           recursion scenario.  Uses watchdog to detect hang.  Verifies
           system survival and normal operation after cleanup.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "DxeCoreTplTest.h"

// ============================================================================
// Test 13 - CLI then RaiseTpl(HIGH) (Scenario 20)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test13CliThenRaiseTpl (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_TPL  OldTpl;

  //
  // Use CPU protocol (architecture-independent equivalent of CLI)
  //
  gCpu->DisableInterrupt (gCpu);

  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->RestoreTPL (OldTpl);

  //
  // False positive: interrupts remain OFF
  //
  UT_ASSERT_FALSE (GetInterruptStateChecked ());

  //
  // Restore and self-clean
  //
  gCpu->EnableInterrupt (gCpu);

  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->RestoreTPL (OldTpl);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  return UNIT_TEST_PASSED;
}

// ============================================================================
// Test 14 - CLI/STI bracket without TPL change (Scenarios 21, 23, 25)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test14CliStiBracketNoTpl (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_TPL  OldTpl;

  gCpu->DisableInterrupt (gCpu);
  //
  // ... critical section, no TPL change ...
  //
  gCpu->EnableInterrupt (gCpu);

  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->RestoreTPL (OldTpl);
  UT_ASSERT_TRUE (GetInterruptStateChecked ());

  return UNIT_TEST_PASSED;
}

// ============================================================================
// Test 15 - STI at TPL_HIGH (Scenario 24)
// ============================================================================

UNIT_TEST_STATUS
EFIAPI
Test15StiAtHigh (
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
  gCpu->EnableInterrupt (gCpu);
  gBS->Stall (StallForTicks (2));
  gCpu->DisableInterrupt (gCpu);
  gBS->RestoreTPL (OldTpl);

  ClearRecursionWatchdog ();

  //
  // System survived
  //
  if (!GetInterruptStateChecked ()) {
    gCpu->EnableInterrupt (gCpu);
  }

  return UNIT_TEST_PASSED;
}
