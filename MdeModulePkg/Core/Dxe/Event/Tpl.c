/** @file
  Task priority (TPL) functions.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeMain.h"
#include "Event.h"

//
// Maximum interrupt nesting depth allowed by the UEFI/PI specifications.
// Comprised of 3 software TPL levels (TPL_APPLICATION, TPL_CALLBACK,
// TPL_NOTIFY) plus 16 hardware interrupt priority levels (TPL 16..31).
//
#define MAX_INTERRUPT_ENABLE_NEST_DEPTH  (3 + 16)

//
// Counter for tracking interrupt enable recursion depth.
// Incremented on entry to CoreSetInterruptState(TRUE) and decremented on exit.
// Prevents stack overflow from infinite interrupt recursion loops.
// Must not exceed MAX_INTERRUPT_ENABLE_NEST_DEPTH per UEFI/PI specs.
//
static volatile UINTN  mInterruptEnableNestDepth = 0;

// Bitmask tracking TPL levels entered from interrupt context.
// Set by CoreTimerTick() on ISR entry.  Used by CoreRestoreTpl() to determine
// when to re-enable interrupts or leave them disabled (the CPU specific
// interrupt-return instruction restores interrupt enables when the ISR
// completes).
//
volatile UINTN  gIsrEntryTplMask = 0;

//
// Interrupted TPL recorded when RaiseTpl(TPL_HIGH_LEVEL) is called while
// interrupts are already disabled (i.e., from ISR context).
//
// This is a single value, not a set.  It is consumed by CoreTimerTick() and
// then cleared to 0.
// CoreRestoreTpl() also clears it when transitioning from HIGH to below HIGH
// to clear any stale non-ISR value.
//
volatile EFI_TPL  gTplBeforeHighTpl = 0;

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

  if (gSmmBase2 != NULL) {
    Status = gSmmBase2->InSmm (gSmmBase2, &InSmm);
    if (EFI_ERROR (Status) || InSmm) {
      return;
    }
  }

  mInterruptEnableNestDepth++;
  ASSERT (mInterruptEnableNestDepth < MAX_INTERRUPT_ENABLE_NEST_DEPTH);

  gCpu->EnableInterrupt (gCpu);

  mInterruptEnableNestDepth--;
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
  // If the new TPL is the same as the current TPL, no action is needed
  //
  if (NewTpl == OldTpl) {
    return NewTpl;
  }

  //
  // If raising to high level, disable interrupts
  //
  if (NewTpl >= TPL_HIGH_LEVEL) {
    if (gCpu != NULL) {
      gCpu->GetInterruptState (gCpu, &State);
      if (!State) {
        //
        // Interrupts are already disabled by hardware (ISR context).
        // Record the OldTpl so CoreTimerTick() can determine the actual
        // interrupted TPL even when the Timer Arch Protocol calls
        // RaiseTpl(TPL_HIGH_LEVEL) before invoking CoreTimerTick().
        //
        // * Timer Arch Protocol raised TPL: OldTpl < HIGH, records the
        //   interrupted level.
        // * Timer Arch Protocol nested (did not raise TPL): OldTpl < HIGH from
        //   dispatch level during CoreReleaseLock's RestoreTpl -- records the
        //   dispatch level.
        // * Normal context: Can occur if code calls DisableInterrupt() then
        //   RaiseTpl(HIGH).  This stale value is cleared by RestoreTpl() when
        //   transitioning from HIGH to below HIGH.
        // * Already at HIGH (lock re-acquire): OldTpl == NewTpl returns above.
        //
        // gTplBeforeHighTpl must be clear. If it is already set, then this is
        // an impossible scenario of an interrupt from TPL_HIGH_LEVEL where
        // all interrupts must be disabled.
        //
        ASSERT (gTplBeforeHighTpl == 0);
        gTplBeforeHighTpl = OldTpl;
      }
    }

    CoreSetInterruptState (FALSE);
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
  // If lowering below HIGH_LEVEL, prepare for interrupt-enabled dispatch.
  //

  if ((OldTpl >= TPL_HIGH_LEVEL) &&  (NewTpl < TPL_HIGH_LEVEL)) {
    gEfiCurrentTpl = TPL_HIGH_LEVEL;
    //
    // Clear any stale gTplBeforeHighTpl value.  In normal context, code may
    // call DisableInterrupt() then RaiseTpl(HIGH), which sets gTplBeforeHighTpl
    // because interrupts are already off.  That value is not meaningful for
    // ISR tracking and must be cleared before event dispatch to avoid confusing
    // a subsequent CoreTimerTick() during nested interrupt handling.
    //
    // This is distinct from CoreTimerTick's consume-and-clear behavior, which
    // clears gTplBeforeHighTpl only after converting a valid ISR entry into
    // gIsrEntryTplMask state.
    //
    gTplBeforeHighTpl = 0;
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
    if (gEfiCurrentTpl < TPL_HIGH_LEVEL) {
      if ((INTN)gEfiCurrentTpl > HighBitSet64 (gIsrEntryTplMask)) {
        CoreSetInterruptState (TRUE);
      }
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
  // Nothing to do if remaining at TPL_HIGH_LEVEL -- interrupts stay disabled.
  //
  if (NewTpl >= TPL_HIGH_LEVEL) {
    return;
  }

  if ((INTN)NewTpl <= HighBitSet64 (gIsrEntryTplMask)) {
    //
    // Still inside an ISR unwind -- clear mask bits for levels that have
    // been fully dispatched and leave interrupts disabled.  The hardware
    // interrupt-return instruction restores the interrupt state.
    //
    gIsrEntryTplMask &= (UINTN)(LShiftU64 (1, NewTpl) - 1);
    return;
  }

  //
  // Normal context (no ISR active) -- re-enable interrupts.
  //
  CoreSetInterruptState (TRUE);
}
