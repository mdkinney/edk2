/** @file
  Task priority (TPL) functions.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeMain.h"
#include "Event.h"

//
// Bitmask tracking TPL levels entered from interrupt context
// This is used by CoreRestoreTpl() to determine when to re-enable interrupts or
// leave them disabled (the CPU specific interrupt-return instruction restores
// interrupt enables when the ISR completes).
//
static volatile UINTN  mIsrEntryTplMask = 0;

/**
  Set Interrupt State.

  @param  Enable  The state of enable or disable interrupt

**/
VOID
CoreSetInterruptState (
  IN BOOLEAN  Enable
  )
{
  EFI_STATUS  Status;
  BOOLEAN     InSmm;

  if (gCpu == NULL) {
    return;
  }

  if (!Enable) {
    gCpu->DisableInterrupt (gCpu);
    return;
  }

  if (gSmmBase2 == NULL) {
    gCpu->EnableInterrupt (gCpu);
    return;
  }

  Status = gSmmBase2->InSmm (gSmmBase2, &InSmm);
  if (!EFI_ERROR (Status) && !InSmm) {
    gCpu->EnableInterrupt (gCpu);
  }
}

/**
  Raise the task priority level to the new level.
  High level is implemented by disabling processor interrupts.

  @param  NewTpl  New task priority level

  @return The previous task priority level

**/
EFI_TPL
EFIAPI
CoreRaiseTpl (
  IN EFI_TPL  NewTpl
  )
{
  EFI_TPL  OldTpl;
  BOOLEAN  State;

  OldTpl = gEfiCurrentTpl;
  if (OldTpl > NewTpl) {
    DEBUG ((DEBUG_ERROR, "FATAL ERROR - RaiseTpl with OldTpl(0x%x) > NewTpl(0x%x)\n", OldTpl, NewTpl));
    ASSERT (FALSE);
  }

  ASSERT (VALID_TPL (NewTpl));

  //
  // If the CPU Arch Protocol is not yet available, interrupts are guaranteed
  // to be disabled per PI Spec Vol 2 — the platform boots with interrupts
  // disabled and they remain so until the CPU and Timer Architectural
  // Protocols are installed.  In that case, no ISR detection is needed, there
  // is no need to disable interrupts, and the TPL can be updated directly.
  //
  if (gCpu != NULL) {
    //
    // If raising to high level, disable interrupts
    //
    if ((NewTpl >= TPL_HIGH_LEVEL) && (OldTpl < TPL_HIGH_LEVEL)) {
      //
      // Query the CPU Arch Protocol for the current hardware interrupt state.
      // If interrupts are already disabled, this call originates from inside an
      // interrupt service routine (ISR).  Timer Arch Protocol conformance
      // requires the ISR to raise to TPL_HIGH_LEVEL before invoking any DXE Core
      // services.
      //
      // All supported CPU architectures guarantee that hardware disables
      // interrupts on interrupt entry (IA-32/X64 interrupt gates clear RFLAGS.IF,
      // AArch64 sets PSTATE.I, RISC-V clears sstatus.SIE, LoongArch clears
      // CSR.CRMD.IE).
      //
      gCpu->GetInterruptState (gCpu, &State);
      if (!State) {
        //
        // Verify ISR nesting is strictly increasing — if a bit is already set
        // at or above OldTpl, the ISR stack invariant has been violated.
        //
        ASSERT ((INTN)OldTpl >= HighBitSet64 (mIsrEntryTplMask));

        //
        // Record the interrupted TPL level to provide CoreRestoreTpl() the
        // information it needs to determine when to re-enable interrupts during
        // event dispatch.  The CPU specific interrupt-return instruction restores
        // the interrupt enables when the ISR completes.
        //
        mIsrEntryTplMask |= (1ULL << OldTpl);
      }

      //
      // Now that ISR detection and tracking is done, disable interrupts.
      //
      CoreSetInterruptState (FALSE);
    }
  }

  //
  // Set the new value
  //
  gEfiCurrentTpl = NewTpl;

  return OldTpl;
}

/**
  Lowers the task priority to the previous value.   If the new
  priority unmasks events at a higher priority, they are dispatched.

  @param  NewTpl  New, lower, task priority

**/
VOID
EFIAPI
CoreRestoreTpl (
  IN EFI_TPL  NewTpl
  )
{
  EFI_TPL  OldTpl;
  EFI_TPL  PendingTpl;

  OldTpl = gEfiCurrentTpl;
  if (NewTpl > OldTpl) {
    DEBUG ((DEBUG_ERROR, "FATAL ERROR - RestoreTpl with NewTpl(0x%x) > OldTpl(0x%x)\n", NewTpl, OldTpl));
    ASSERT (FALSE);
  }

  ASSERT (VALID_TPL (NewTpl));

  //
  // If lowering below HIGH_LEVEL, make sure
  // interrupts are enabled
  //

  if ((OldTpl >= TPL_HIGH_LEVEL) &&  (NewTpl < TPL_HIGH_LEVEL)) {
    gEfiCurrentTpl = TPL_HIGH_LEVEL;
  }

  //
  // Dispatch any pending events
  //
  while (gEventPending != 0) {
    PendingTpl = (UINTN)HighBitSet64 (gEventPending);
    if (PendingTpl <= NewTpl) {
      break;
    }

    gEfiCurrentTpl = PendingTpl;

    //
    // Allow preemption: re-enable interrupts if dispatching above all
    // interrupted levels.  A new timer interrupt can then preempt this
    // lower-priority event handler to service higher-priority events.
    //
    if ((HighBitSet64 (mIsrEntryTplMask) < (INTN)gEfiCurrentTpl) &&
        (gEfiCurrentTpl < TPL_HIGH_LEVEL))
    {
      CoreSetInterruptState (TRUE);
    }

    CoreDispatchEventNotifies (gEfiCurrentTpl);
  }

  //
  // Disable interrupts before committing NewTpl.  This closes a race window
  // where an interrupt could see the lowered gEfiCurrentTpl before the
  // interrupt-enable decision below is made.
  //
  CoreSetInterruptState (FALSE);

  //
  // Set the new value
  //
  gEfiCurrentTpl = NewTpl;

  //
  // Nothing to do if remaining at TPL_HIGH_LEVEL — interrupts stay disabled.
  //
  if (NewTpl >= TPL_HIGH_LEVEL) {
    return;
  }

  if ((INTN)NewTpl <= HighBitSet64 (mIsrEntryTplMask)) {
    //
    // Still inside an ISR unwind — clear mask bits for levels that have
    // been fully dispatched and leave interrupts disabled.  The hardware
    // interrupt-return instruction will restore them.
    //
    mIsrEntryTplMask &= (1ULL << NewTpl) - 1;
    return;
  }

  //
  // Normal context (no ISR active) — re-enable interrupts.
  //
  CoreSetInterruptState (TRUE);
}
