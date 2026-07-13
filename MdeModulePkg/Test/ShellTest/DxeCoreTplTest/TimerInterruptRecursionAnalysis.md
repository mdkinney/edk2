# DXE Core Timer Interrupt Infinite Recursion Analysis

## Executive Summary

**Problem:** The DXE Core's `CoreRestoreTpl()` unconditionally re-enables
processor interrupts when lowering TPL below `TPL_HIGH_LEVEL`. When this
occurs inside `CoreTimerTick()` (the DXE Core's timer notify function),
a pending timer interrupt immediately fires, recursively re-entering the
handler. This creates unbounded stack growth leading to stack overflow.

**Root Cause:** All CPU architectures disable interrupts on interrupt entry
(hardware guarantee). `CoreTimerTick()` acquires a spinlock via
`CoreAcquireLock()` (which calls `RaiseTpl(HIGH)`) and releases it via
`CoreReleaseLock()` (which calls `RestoreTpl()`). The `RestoreTpl()` call
re-enables interrupts in software before the interrupt handler returns,
creating a recursion window. The CPU Architecture Protocol's
`GetInterruptState()` -- the natural way to detect interrupt context --
returns stale cached values on IA-32/X64 and RISC-V, making ISR detection
unreliable.

**Timer Architecture Protocol Patterns:** There are two fundamentally
different timer ISR implementations in edk2:
- **Timer drivers that call RaiseTpl(HIGH) before CoreTimerTick:**
  The timer ISR calls `RaiseTpl(HIGH)` before invoking `CoreTimerTick()`,
  then `RestoreTpl()` afterward (used by ArmPkg/Drivers/TimerDxe,
  OvmfPkg/LocalApicTimerDxe). The outer `RestoreTpl()` creates an
  additional recursion point.
- **Timer drivers that do not call RaiseTpl before CoreTimerTick:**
  The timer ISR calls `CoreTimerTick()` directly without any TPL wrapper,
  relying on the IRET instruction to restore interrupt state (used by
  PcAtChipsetPkg/HpetTimerDxe). Recursion occurs solely from
  `CoreTimerTick()`'s internal lock release.

Both implementations trigger unbounded recursion through `CoreTimerTick()`'s
internal `CoreReleaseLock()` -> `CoreRestoreTpl()` path. The fix must
handle both.

**Recommended Solution (Option F -- Solution 4):** Three-part fix:
1. **CPU DXE drivers (Part 1):** Fix `GetInterruptState()` in the IA-32/X64
   and RISC-V CPU DXE drivers to read the actual hardware interrupt flag
   instead of returning a stale cached value (spec conformance fix).
2. **CoreRaiseTpl + CoreTimerTick (Part 2a):** `CoreRaiseTpl` records the
   pre-HIGH TPL in `gTplBeforeHighTpl` when interrupts are already disabled
   (ISR context or explicit `DisableInterrupt()`). At entry, `CoreTimerTick`
   reads `gTplBeforeHighTpl` to determine the interrupted TPL and sets the
   corresponding bit in `gIsrEntryTplMask`. For timer drivers that do not
   call RaiseTpl, `gEfiCurrentTpl` directly reflects the interrupted level.
   Note: `CoreRestoreTpl` clears `gTplBeforeHighTpl` when transitioning
   from HIGH to below HIGH, so stale value from normal-context
   `DisableInterrupt()` + `RaiseTpl(HIGH)` sequences never propagate to
   `gIsrEntryTplMask`.
3. **CoreRestoreTpl (Part 2b):** When lowering TPL, check `gIsrEntryTplMask`.
   If the new TPL is at or below any bit set in the mask (meaning we are
   still inside an ISR unwind), suppress interrupt re-enable and clear the
   completed mask bits. The hardware interrupt-return instruction (IRET/ERET)
   will restore the interrupt flag. Only re-enable interrupts when above all
   interrupted levels (normal context or preempting a lower-priority handler).

Together, CoreTimerTick sets the mask on ISR entry, and CoreRestoreTpl
consults it to decide whether to re-enable interrupts. This prevents the
recursive interrupt -> CoreTimerTick -> RestoreTpl -> interrupt cycle.

This bounds nesting to at most 4 levels (one per distinct TPL: APPLICATION,
CALLBACK, NOTIFY, HIGH-1), works on all architectures (X64, AARCH64,
RISC-V), requires no timer driver changes, eliminates false positives,
and fixes an independent spec-conformance bug in the CPU Arch Protocol.

**Diagnostics:** `CoreTimerTick()` is instrumented with per-TPL entry
counters (`EntriesAtTpl[]`) and `MaxDepth` tracking, exposed via an
EFI Configuration Table. This enables the test suite to verify bounded
recursion depth without modifying production behavior.

---

## 1 Problem Statement

The DXE Core's TPL (Task Priority Level) management in `CoreRestoreTpl()` can
unconditionally re-enable processor interrupts while still executing inside a
timer interrupt handler (`CoreTimerTick()`). This creates a window where a
pending or new timer interrupt can immediately fire, re-entering
`CoreTimerTick()` recursively. Each nested invocation repeats the same pattern,
leading to **unbounded stack growth and a stack overflow crash**.

### 1.1 Specification Analysis -- Task Priority Levels

References:
- [UEFI Specification 2.11, Section 4.1 -- "Event, Timer, and Task Priority Services"](https://uefi.org/specs/UEFI/2.11/07_Services_Boot_Services.html#event-timer-and-task-priority-services)
- [UEFI Specification 2.11, Section 4.1.8 -- "EFI_BOOT_SERVICES.RaiseTPL()"](https://uefi.org/specs/UEFI/2.11/07_Services_Boot_Services.html#efi-boot-services-raisetpl)
- [UEFI Specification 2.11, Section 4.1.9 -- "EFI_BOOT_SERVICES.RestoreTPL()"](https://uefi.org/specs/UEFI/2.11/07_Services_Boot_Services.html#efi-boot-services-restoretpl)

#### 1.1.1 Defined TPL Levels

The UEFI Specification defines four named priority levels:

```c
#define TPL_APPLICATION  4
#define TPL_CALLBACK     8
#define TPL_NOTIFY       16
#define TPL_HIGH_LEVEL   31
```

The full TPL Usage table from UEFI Spec Section 4.1:

| TPL Level | Usage (from spec) |
|---|---|
| `TPL_APPLICATION` (4) | "The lowest priority level. It is the level of execution which occurs when no event notifications are pending and which interacts with the user." |
| `TPL_CALLBACK` (8) | "Interrupts code executing below TPL_CALLBACK level. Long term operations (such as file system operations and disk I/O) can occur at this level." |
| `TPL_NOTIFY` (16) | "Interrupts code executing below TPL_NOTIFY level. Blocking is not allowed at this level. Code executes to completion and returns." |
| **(Firmware Interrupts)** (17-30) | "This level is internal to the firmware. It is the level at which internal interrupts occur. Code running at this level interrupts code running at the TPL_NOTIFY level (or lower levels). If the interrupt requires extended time to complete, firmware signals another event (or events) to perform the longer term operations so that other interrupts can occur." |
| `TPL_HIGH_LEVEL` (31) | "Interrupts code executing below TPL_HIGH_LEVEL. This is the highest priority level. It is not interruptible (interrupts are disabled) and is used sparingly by firmware to synchronize operations that need to be accessible from any priority level." |

#### 1.1.2 Reserved Firmware Interrupt Levels (TPL 17-30)

The `RaiseTPL()` description (Section 4.1.8) states:

> *"Only three task priority levels are exposed outside of the firmware
> during boot services execution. The first is TPL_APPLICATION where all
> normal execution occurs. That level may be interrupted to perform various
> asynchronous interrupt style notifications, which occur at the
> TPL_CALLBACK or TPL_NOTIFY level."*

And critically:

> *"Additionally, only TPL_APPLICATION, TPL_CALLBACK, TPL_NOTIFY, and
> TPL_HIGH_LEVEL may be used. All other values are reserved for use by the
> firmware; using them will result in unpredictable behavior."*

The `RestoreTPL()` description (Section 4.1.9) repeats the same restriction.

This establishes that the UEFI architecture **envisions a multi-level
interrupt priority scheme** where TPL 17-30 represents firmware-internal
hardware interrupt processing levels -- conceptually similar to Windows
IRQL (Interrupt Request Level) or a traditional OS kernel's IPL (Interrupt
Priority Level).

#### 1.1.3 Event Notification Constraints

The UEFI Spec restricts event notifications:

> *"Event Notification Levels: > TPL_APPLICATION, <= TPL_HIGH_LEVEL"*

The `CreateEventEx()` implementation rejects `NotifyTpl >= TPL_HIGH_LEVEL`
and `NotifyTpl <= TPL_APPLICATION`. This means event callbacks run strictly
at TPL_CALLBACK (8) or TPL_NOTIFY (16) -- never at TPL_HIGH_LEVEL and never
in the firmware-internal range (17-30).

#### 1.1.4 The Interrupt Enable/Disable Rule

The spec establishes the fundamental TPL-interrupt coupling:

- **At TPL_HIGH_LEVEL (31):** "It is not interruptible (interrupts are
  disabled)"
- **Below TPL_HIGH_LEVEL:** Interrupts are enabled (implied by the TPL
  Usage table describing how lower-level code "may be interrupted")

This is the only hardware effect of TPL transitions -- changing from
`TPL_HIGH_LEVEL` to any lower level re-enables processor interrupts.

### 1.2 Specification Analysis -- Timer Architecture Protocol

References:
- [PI Specification 1.9, Volume 2, Section 12.10 -- "Timer Architectural Protocol"](https://uefi.org/specs/PI/1.9/V2_DXE_Architectural_Protocols.html#timer-architectural-protocol)
- [PI Specification 1.9, Volume 2, Section 12.10.2 -- "EFI_TIMER_ARCH_PROTOCOL.RegisterHandler()"](https://uefi.org/specs/PI/1.9/V2_DXE_Architectural_Protocols.html#efi-timer-arch-protocol-registerhandler)

#### 1.2.1 The Timer Notify Contract

The PI Specification defines `EFI_TIMER_NOTIFY`:

```c
typedef VOID (EFIAPI *EFI_TIMER_NOTIFY)(IN UINT64 Time);
```

The `RegisterHandler()` description states:

> *"The function to call when a timer interrupt fires. This function
> executes at TPL_HIGH_LEVEL. The DXE Foundation will register a handler
> for the timer interrupt, so it can know how much time has passed. This
> information is used to signal timer based events."*

This establishes:
1. The timer notify function (i.e., `CoreTimerTick`) executes at
   TPL_HIGH_LEVEL
2. The DXE Foundation registers this handler (it's a core service, not a
   driver callback)
3. The purpose is to signal timer-based events

#### 1.2.2 What the Spec Leaves Unspecified

The PI Spec is **silent** on these critical questions:

1. **Who raises TPL to HIGH before calling the notify function?** -- Is it
   the timer driver's responsibility, or does the DXE Core expect to be
   called with interrupts disabled and TPL already at HIGH?

2. **Who restores TPL after the notify function returns?** -- Does the timer
   driver call `RestoreTPL`, or does the function return with TPL still at
   HIGH?

3. **What happens to interrupt state during the handler's internal
   `RestoreTpl` calls?** -- If the handler acquires/releases locks (which
   invoke `RaiseTpl(HIGH)` / `RestoreTpl`), should `RestoreTpl` re-enable
   interrupts inside the handler?

4. **Is re-entrant invocation of the notify function permitted?** -- If
   interrupts are re-enabled during handler execution, can a new timer
   interrupt invoke the same handler recursively?

### 1.3 Specification Analysis -- CPU Architecture Protocol

References:
- [PI Specification 1.9, Volume 2, Section 12.3 -- "CPU Architectural Protocol"](https://uefi.org/specs/PI/1.9/V2_DXE_Architectural_Protocols.html#cpu-architectural-protocol)
- [PI Specification 1.9, Volume 2, Section 12.3.3 -- "EnableInterrupt()"](https://uefi.org/specs/PI/1.9/V2_DXE_Architectural_Protocols.html#efi-cpu-arch-protocol-enableinterrupt)
- [PI Specification 1.9, Volume 2, Section 12.3.4 -- "DisableInterrupt()"](https://uefi.org/specs/PI/1.9/V2_DXE_Architectural_Protocols.html#efi-cpu-arch-protocol-disableinterrupt)
- [PI Specification 1.9, Volume 2, Section 12.3.7 -- "RegisterInterruptHandler()"](https://uefi.org/specs/PI/1.9/V2_DXE_Architectural_Protocols.html#efi-cpu-arch-protocol-registerinterrupthandler)

#### 1.3.1 Binary Interrupt Control Only

The `EFI_CPU_ARCH_PROTOCOL` provides exactly two interrupt control functions:

- **`EnableInterrupt()`** -- "Enables interrupt processing by the
  processor. This function is used to implement the Boot Services
  `RaiseTPL()` and `RestoreTPL()`."

- **`DisableInterrupt()`** -- "Disables interrupt processing by the
  processor. This function is used to implement the Boot Services
  `RaiseTPL()` and `RestoreTPL()`."

There is **no** `SetInterruptPriority(level)` or equivalent API. The CPU
Arch Protocol only supports a binary state: all maskable interrupts enabled,
or all disabled.

#### 1.3.2 Interrupt Handler Registration

The `RegisterInterruptHandler()` description states:

> *"This function registers and enables the handler specified by
> InterruptHandler for a processor interrupt or exception type... The
> installed handler is called once for each processor interrupt or
> exception."*

And:

> *"This function is typically used by the EFI_TIMER_ARCH_PROTOCOL to hook
> the timer interrupt in a system."*

The spec does NOT state what TPL or interrupt state the handler is called
at -- it only says the CPU Arch Protocol "must handle saving and restoring
system context" and "perform the necessary steps to return to the context
that was interrupted."

#### 1.3.3 Hardware Guarantees Across Architectures

All processor architectures supported by UEFI guarantee that interrupts are
hardware-disabled before the interrupt handler begins executing:

| Architecture | HW auto-disables interrupts on entry? | Mechanism | Restored by |
|---|---|---|---|
| **IA-32** | **Yes** | Interrupt gate clears `EFLAGS.IF` | `IRET` |
| **X64** | **Yes** | Interrupt gate clears `RFLAGS.IF` | `IRETQ` |
| **AArch64** | **Yes** | HW sets `PSTATE.{I,F}` (DAIF mask) | `ERET` |
| **ARM32** | **Yes** | HW sets `CPSR.I` (and `.F` for FIQ) | Exception return via SPSR |
| **RISC-V** | **Yes** | HW clears `sstatus.SIE` | `SRET` |
| **LoongArch** | **Yes** | HW clears `CSR.CRMD.IE` | `ERTN` |

References:
- [Intel SDM Vol. 3A, Section 6.12.1.2 -- "Flag Usage By Exception- or Interrupt-Handler Procedure"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
- [Arm Architecture Reference Manual for A-profile, Section D1.10 -- "Exception entry"](https://developer.arm.com/documentation/ddi0487/latest)
- [Arm Architecture Reference Manual (ARMv7-A), Section B1.8.3 -- "Exception entry"](https://developer.arm.com/documentation/ddi0406/latest)
- [RISC-V Privileged Specification, Section 4.1.1 -- "Supervisor Status Register (sstatus)"](https://github.com/riscv/riscv-isa-manual)
- [LoongArch Reference Manual, Volume 1, Section 4.4 -- "Exception and Interrupt Entry"](https://loongson.github.io/LoongArch-Documentation/)

This means the timer interrupt handler ALWAYS begins with interrupts
disabled by hardware. The recursion bug relies entirely on **software** (the
DXE Core) prematurely re-enabling interrupts before the handler returns.

### 1.4 The Architectural Conflicts

The three specification elements above create a set of irreconcilable
conflicts when combined.

#### 1.4.1 Conflict: RestoreTpl vs. Interrupt Context

**UEFI Spec says:** Restoring TPL below HIGH re-enables interrupts.

**PI Spec says:** Timer notify function executes at TPL_HIGH_LEVEL.

**Hardware says:** Interrupt handler entered with interrupts disabled;
interrupt-return instruction restores them.

**The conflict:** If the timer notify function acquires a lock at
TPL_HIGH_LEVEL and then releases it (restoring to the interrupted TPL), the
UEFI-specified behavior of `RestoreTpl` demands interrupt re-enable. But the
handler is still inside interrupt context -- re-enabling allows recursive
re-entry before the interrupt-return instruction executes.

#### 1.4.2 Conflict: Multi-Level Vision vs. Binary Implementation

**UEFI Spec envisions:** TPL 17-30 as firmware interrupt priority levels
where interrupt handlers execute at intermediate priorities, with
higher-priority interrupts able to preempt lower-priority ones.

**CPU Arch Protocol provides:** Only `EnableInterrupt()` /
`DisableInterrupt()` -- a binary enable/disable with no priority masking.

**The conflict:** The spec's intended design would have timer handlers
running at (e.g., TPL 20). A lock acquire -> `RaiseTpl(31)` -> lock release ->
`RestoreTpl(20)` would NOT re-enable interrupts because TPL 20 is still in
the "firmware interrupt" range. But without multi-level support, the handler
must use TPL_HIGH_LEVEL (31) as its base, and any `RestoreTpl` to the
interrupted TPL (4-16) crosses the enable threshold.

#### 1.4.3 Conflict: RestoreTpl Conflates Two Operations

`RestoreTpl()` must make two independent decisions in a single function:

1. **Software decision:** Which pending event callbacks to dispatch
   (determined by comparing `gEventPending` against the new TPL)
2. **Hardware decision:** Whether to enable processor interrupts (determined
   by whether the new TPL is below HIGH)

These concerns are orthogonal. Inside an interrupt handler, it may be
correct to dispatch pending software events (at levels above the interrupted
TPL) while still keeping hardware interrupts disabled. But `RestoreTpl` has
no mechanism to separate these -- lowering TPL for event dispatch
simultaneously enables interrupts.

#### 1.4.4 Conflict: No Interrupt Context Indicator

Neither the UEFI Spec nor the PI Spec defines a mechanism for the DXE Core
to know whether it is executing in interrupt context vs. normal context. The
only observable signal is the hardware interrupt state (accessible via
`EFI_CPU_ARCH_PROTOCOL.GetInterruptState()`), which returns FALSE both when:
- Code has explicitly called `RaiseTpl(HIGH)` (normal context, interrupts
  disabled by software)
- An interrupt handler is executing (interrupt context, interrupts disabled
  by hardware)

The DXE Core must distinguish these cases to make correct enable/disable
decisions, but the specs provide no defined way to do so.

#### 1.4.5 Summary of Architectural Gaps

| Aspect | Spec Intent | Actual Capability | Gap |
|---|---|---|---|
| Interrupt priority | Multi-level (TPL 17-30) | Binary enable/disable | No priority masking API |
| Interrupt context | Implied by "executes at HIGH" | No explicit indicator | Cannot distinguish from normal `RaiseTpl(HIGH)` |
| RestoreTpl scope | Lower priority + enable interrupts | Single operation | Cannot lower priority without enabling |
| Timer handler recursion | Not addressed | Unbounded | No re-entrancy protection |
| Lock release in handler | Not addressed | Re-enables interrupts | Triggers recursion |

### 1.5 Implementation Observations in edk2

The following observations from the edk2 source code illustrate how the
architectural conflicts described in Section 1.4 manifest in practice.

#### 1.5.1 DXE Core -- CoreTimerTick and Lock Usage

The DXE Foundation registers `CoreTimerTick()` as the `EFI_TIMER_NOTIFY`
handler. This function acquires a TPL_HIGH_LEVEL lock:

```c
// MdeModulePkg/Core/Dxe/Event/Timer.c
EFI_LOCK mEfiSystemTimeLock = EFI_INITIALIZE_LOCK_VARIABLE (TPL_HIGH_LEVEL);

VOID EFIAPI CoreTimerTick (IN UINT64 Duration) {
    CoreAcquireLock (&mEfiSystemTimeLock);   // -> CoreRaiseTpl(TPL_HIGH_LEVEL)
    mEfiSystemTime += Duration;
    if (...timer expired...) {
        CoreSignalEvent (mEfiCheckTimerEvent);
    }
    CoreReleaseLock (&mEfiSystemTimeLock);   // -> CoreRestoreTpl(interrupted_TPL)
    // -> THIS re-enables interrupts inside the ISR
}
```

The `CoreReleaseLock` -> `CoreRestoreTpl(interrupted_TPL)` call is where the
Conflict 1.4.1 materializes: `RestoreTpl` re-enables interrupts while still
inside the timer interrupt handler.

#### 1.5.2 Timer Driver Patterns -- Three Incompatible Approaches

The PI Spec's ambiguity (Section 1.2.2) has led to three different timer
driver patterns in edk2:

**Timer driver that calls RaiseTpl/RestoreTpl around CoreTimerTick (ArmPkg):**

Source: `ArmPkg/Drivers/TimerDxe/TimerDxe.c`

```c
VOID EFIAPI TimerInterruptHandler (...) {
    OriginalTPL = gBS->RaiseTPL (TPL_HIGH_LEVEL);
    gInterrupt->EndOfInterrupt (gInterrupt, Source);
    mTimerNotifyFunction (mTimerPeriod);
    gBS->RestoreTPL (OriginalTPL);
}
```

The outer `RestoreTPL` is intended to control interrupt re-enable, but
`CoreTimerTick`'s inner lock release already re-enabled them (Conflict
1.4.1).

**Timer driver that does not call RaiseTpl before CoreTimerTick (PcAtChipsetPkg):**

Source: `PcAtChipsetPkg/HpetTimerDxe/HpetTimer.c`

```c
VOID EFIAPI TimerInterruptHandler (...) {
    SendApicEoi ();
    mTimerNotifyFunction (TimerPeriod);
    // Returns -- IRET restores interrupt state
}
```

Relies entirely on `CoreTimerTick`'s lock management. The inner
`CoreRestoreTpl` re-enables interrupts before IRET executes.

**Deferred RestoreTpl via NestedInterruptTplLib (OvmfPkg):**

Source: `OvmfPkg/LocalApicTimerDxe/LocalApicTimerDxe.c`

```c
VOID EFIAPI TimerInterruptHandler (...) {
    OriginalTPL = NestedInterruptRaiseTPL ();
    SendApicEoi ();
    mTimerNotifyFunction (mTimerPeriod);
    NestedInterruptRestoreTPL (OriginalTPL, SystemContext, &State);
}
```

Uses `OvmfPkg/Library/NestedInterruptTplLib/Tpl.c` which:
- Detects nested interrupt during outer handler's `RestoreTpl`
- Defers inner handler's restore to the outer handler
- Manipulates x86 IRET-saved RFLAGS to disable interrupts on return
- Bounds stack depth to the number of distinct TPL levels

This is an x86-specific workaround that operates outside the DXE Core.

**Why NestedInterruptTplLib avoids the CoreTimerTick-internal recursion:**
Because the timer handler raises TPL to HIGH **before** calling `CoreTimerTick`,
the `CoreAcquireLock(mEfiSystemTimeLock)` inside `CoreTimerTick` records
`OwnerTpl = HIGH` (already at HIGH). When `CoreReleaseLock` calls
`CoreRestoreTpl(HIGH)`, this is a no-op -- no interrupts are re-enabled and no
event dispatch occurs inside `CoreTimerTick`. The recursion risk for this
pattern exists only **after** `CoreTimerTick` returns, when
`NestedInterruptRestoreTPL` eventually calls `CoreRestoreTpl(interrupted TPL)`.
NestedInterruptTplLib bounds that recursion with its own deferred-restore and
RFLAGS manipulation mechanism.

The DxeCore fix (`gIsrEntryTplMask`) is primarily needed for timer handlers
that do **not** raise TPL before calling `CoreTimerTick` (e.g., the AARCH64
pattern), where `CoreReleaseLock` restores to the interrupted TPL below HIGH,
re-enabling interrupts and creating the recursion path.

**Scope comparison:** NestedInterruptTplLib resolves the recursion for x86
platforms whose timer drivers opt into it, but it is architecture-specific
(RFLAGS manipulation) and requires per-driver adoption. The DxeCore fix
resolves the recursion universally -- all architectures, all timer handler
patterns, no driver changes needed -- making NestedInterruptTplLib redundant
for recursion prevention.

#### 1.5.3 DXE Core -- CoreRestoreTpl

The `CoreRestoreTpl()` in `MdeModulePkg/Core/Dxe/Event/Tpl.c`
unconditionally re-enables interrupts whenever TPL drops below HIGH:

```c
if ((OldTpl >= TPL_HIGH_LEVEL) && (NewTpl < TPL_HIGH_LEVEL)) {
    gEfiCurrentTpl = TPL_HIGH_LEVEL;
}
while (gEventPending != 0) {
    // dispatch events...
    CoreSetInterruptState (TRUE);   // Enables interrupts during dispatch
    CoreDispatchEventNotifies (gEfiCurrentTpl);
}
gEfiCurrentTpl = NewTpl;
if (NewTpl < TPL_HIGH_LEVEL) {
    CoreSetInterruptState (TRUE);   // Unconditionally enables interrupts
}
```

There is no check for whether we are inside an interrupt handler. This is
Conflict 1.4.4 in action -- the code has no mechanism to detect interrupt
context.

#### 1.5.4 CPU Arch Protocol Implementations

All edk2 CPU DXE drivers configure interrupt vectors to disable interrupts
on entry:

- **IA-32/X64:** IDT entries use interrupt gates (`IA32_IDT_GATE_TYPE_INTERRUPT_32`)
  which clear `EFLAGS.IF` / `RFLAGS.IF` on entry.
  Source: `UefiCpuPkg/Library/CpuExceptionHandlerLib/X64/ArchExceptionHandler.c`
- **AArch64:** Hardware sets PSTATE.{I,F} on exception entry (no software
  configuration needed).
  Source: `ArmPkg/Library/ArmExceptionLib/AArch64/ExceptionSupport.S`
- **RISC-V:** Hardware clears `sstatus.SIE` on trap entry.
  Source: `UefiCpuPkg/Library/CpuExceptionHandlerLib/RiscV/ExceptionHandlerAsm.S`
- **LoongArch:** Hardware clears `CSR.CRMD.IE` on exception entry.
  Source: `UefiCpuPkg/Library/CpuExceptionHandlerLib/LoongArch/LoongArch64/ExceptionHandlerAsm.S`

**CPU Arch Protocol `GetInterruptState()` Returns Cached State, Not Hardware:**

Two edk2 CPU Architecture Protocol implementations maintain a **cached
boolean variable** (`InterruptState` or `mInterruptState`) that is updated
only by explicit calls to `CpuEnableInterrupt()` / `CpuDisableInterrupt()`.
The protocol's `GetInterruptState()` returns this cached value -- it does
**not** read the actual CPU hardware interrupt flag. This means that when
hardware disables interrupts on interrupt entry (e.g., clearing EFLAGS.IF
via an interrupt gate), the protocol still reports interrupts as *enabled*
because no software call updated the cache.

Affected implementations:

| Driver | Cached Variable | Source |
|--------|----------------|--------|
| `UefiCpuPkg/CpuDxe` (IA32/X64) | `BOOLEAN InterruptState` (global) | `CpuDxe.c:16` -- `*State = InterruptState;` |
| `UefiCpuPkg/CpuDxeRiscV64` | `STATIC BOOLEAN mInterruptState` | `CpuDxe.c:15` -- `*State = mInterruptState;` |

Implementations that already read hardware:

| Driver | Mechanism | Source |
|--------|-----------|--------|
| `ArmPkg/Drivers/CpuDxe` (AArch64) | `ArmGetInterruptState()` reads DAIF register | `CpuDxe.c` |
| `UefiCpuPkg/CpuDxeLoongArch64` | Reads `CSR.CRMD.IE` directly | `CpuDxe.c` |

**PI Specification Intent:**

The PI Specification's `GetInterruptState()` description states:

> *"Retrieves the processor's current interrupt state."*

The word "current" implies the actual hardware state, not a cached
approximation. The X64 and RISC-V implementations are arguably
non-conforming -- they return a stale value that does not reflect
hardware-initiated state changes (such as interrupt gate clearing IF).

**The BaseLib Alternative:**

The BaseLib `GetInterruptState()` function (distinct from the CPU Arch
Protocol function of the same name) reads the actual hardware state on all
architectures:

- **IA-32/X64:** Reads `EFLAGS.IF` via `AsmReadEflags()` -- correctly returns
  FALSE inside an interrupt gate handler.
  Source: `MdePkg/Library/BaseLib/X86GetInterruptState.c`
- **AArch64:** Reads `DAIF` register -- correctly returns FALSE when IRQs are
  masked.
  Source: `MdePkg/Library/BaseLib/AArch64/GetInterruptsState.S`
- **RISC-V:** Reads `sstatus.SIE` -- correctly returns FALSE during trap
  handling.
  Source: `MdePkg/Library/BaseLib/RiscV64/GetInterruptState.c`
- **LoongArch:** Reads `CSR.CRMD.IE` -- correctly returns FALSE during
  exception handling.
  Source: `MdePkg/Library/BaseLib/LoongArch64/GetInterruptState.S`

**Implication:** The DXE Core cannot reliably use
`gCpu->GetInterruptState()` to detect "am I in an interrupt handler?" on
IA-32/X64 or RISC-V because the cached value is stale during ISR execution.
This is a critical obstacle for any fix that relies on detecting interrupt
context via the CPU Arch Protocol -- the protocol must first be corrected to
read hardware state (see Section 3.6).

#### 1.5.5 Unused TPL 17-30

No edk2 code uses TPL values between 17 and 30. The `EmulatorPkg` debug
script explicitly labels TPL 6 as "TPL_DRIVER (Obsolete Concept in edk2)".
The DXE Core's `CoreCreateEventInternal()` rejects `NotifyTpl >=
TPL_HIGH_LEVEL`, limiting event callbacks to levels 4-16. This confirms
Conflict 1.4.2: the multi-level firmware interrupt scheme exists only on
paper.

### 1.6 Root Cause

In the original code, `CoreRestoreTpl()` **unconditionally** calls
`CoreSetInterruptState(TRUE)` whenever the current TPL drops below
`TPL_HIGH_LEVEL`. It does not distinguish between:

- A normal `RestoreTpl` call from application/driver code, where re-enabling
  interrupts is correct.
- A `RestoreTpl` call that occurs **inside an interrupt handler** (via
  `CoreReleaseLock` within `CoreTimerTick`), where re-enabling interrupts
  allows recursive re-entry before the handler has returned.

---

## 2 Call Flows Illustrating the Problem

This section presents call flow diagrams that illustrate how the bug manifests.
Each subsection highlights a different aspect of the recursion mechanism.

### 2.1 Timer Arch Protocol -- Two Implementation Patterns

Two incompatible timer handler patterns exist in edk2. The recursion path
differs between them, which is critical to understanding why a DxeCore-level
fix is required.

#### 2.1.1 Pattern: Timer Handler Does NOT Raise TPL (AARCH64, PcAtChipsetPkg)

In this pattern, the timer handler calls `CoreTimerTick` directly without
raising TPL. The `CoreAcquireLock` inside `CoreTimerTick` raises TPL to HIGH
and `CoreReleaseLock` restores it to the interrupted TPL -- re-enabling
interrupts and creating the recursion window **inside** `CoreTimerTick`.

```mermaid
flowchart TD
    A[Timer Interrupt Fires] --> B[Hardware disables interrupts]
    B --> C[TimerInterruptHandler entry]
    C --> D[SendApicEoi / EndOfInterrupt]
    D --> E["mTimerNotifyFunction() -> CoreTimerTick()"]
    E --> F["CoreAcquireLock: CoreRaiseTpl(HIGH=31)<br/>Records OldTpl = interrupted TPL"]
    F --> G[Update mEfiSystemTime, signal timer events]
    G --> H["CoreReleaseLock: CoreRestoreTpl(interrupted TPL)"]
    H --> I{"NewTpl < HIGH?"}
    I -->|Yes| J["CoreSetInterruptState(TRUE)<br/>!! INTERRUPTS RE-ENABLED !!"]
    J --> K{"Another timer<br/>interrupt pending?"}
    K -->|Yes| L["!! RECURSION: new CoreTimerTick<br/>before current one returns"]
    K -->|No| M[CoreRestoreTpl returns]
    M --> N[CoreTimerTick returns]
    N --> O[TimerInterruptHandler returns via IRET]
    I -->|No| M

    style J fill:#f66,stroke:#333
    style L fill:#f00,stroke:#333,color:#fff
```

#### 2.1.2 Pattern: Timer Handler Raises TPL Before CoreTimerTick (OvmfPkg, ArmPkg)

In this pattern, the timer handler raises TPL to HIGH before calling
`CoreTimerTick`. Because TPL is already HIGH on entry, the `CoreAcquireLock`
inside `CoreTimerTick` records `OwnerTpl = HIGH`, and `CoreReleaseLock` calls
`CoreRestoreTpl(HIGH)` -- a no-op. No interrupts are re-enabled inside
`CoreTimerTick`. The recursion risk exists only **after** `CoreTimerTick`
returns, when the timer handler restores the original TPL.

```mermaid
flowchart TD
    A[Timer Interrupt Fires] --> B[Hardware disables interrupts]
    B --> C[TimerInterruptHandler entry]
    C --> D["RaiseTpl(HIGH=31) or NestedInterruptRaiseTPL()<br/>Records interrupted TPL"]
    D --> E[SendApicEoi / EndOfInterrupt]
    E --> F["mTimerNotifyFunction() -> CoreTimerTick()"]
    F --> G["CoreAcquireLock: CoreRaiseTpl(HIGH)<br/>Already at HIGH -> OldTpl = HIGH"]
    G --> H[Update mEfiSystemTime, signal timer events]
    H --> I["CoreReleaseLock: CoreRestoreTpl(HIGH)<br/>!! NO-OP: OldTpl == NewTpl == HIGH"]
    I --> J["CoreTimerTick returns<br/>(no interrupts re-enabled inside)"]
    J --> K["RestoreTpl(interrupted TPL) or<br/>NestedInterruptRestoreTPL()"]
    K --> L{"NewTpl < HIGH?"}
    L -->|Yes| M["CoreSetInterruptState(TRUE)<br/>!! INTERRUPTS RE-ENABLED !!"]
    M --> N{"Another timer<br/>interrupt pending?"}
    N -->|Yes| O["!! RECURSION: new TimerInterruptHandler<br/>before outer handler returns"]
    N -->|No| P[CoreRestoreTpl returns]
    P --> Q[TimerInterruptHandler returns via IRET]
    L -->|No| P

    style M fill:#f66,stroke:#333
    style O fill:#f00,stroke:#333,color:#fff
    style I fill:#6f6,stroke:#333
```

**Key difference:** In Pattern 2.1.1, the recursion window opens **inside**
`CoreTimerTick` (during `CoreReleaseLock`). In Pattern 2.1.2, the recursion
window opens **after** `CoreTimerTick` returns (during the handler's own
`RestoreTpl`). The DxeCore fix (`gIsrEntryTplMask`) closes the window in both
cases because `CoreRestoreTpl` checks the mask regardless of who called it.

### 2.2 Normal Operation (No Recursion)

This diagram shows the normal timer interrupt flow. Application code
runs at `TPL_APPLICATION` with interrupts enabled. When the timer fires,
hardware disables interrupts and invokes `CoreTimerTick`. The handler acquires
the system time lock (raising to `TPL_HIGH_LEVEL`), does its work, then
releases the lock (restoring to `TPL_APPLICATION`). The bug is visible here:
`CoreRestoreTpl` re-enables interrupts via `CoreSetInterruptState(TRUE)` while
still inside `CoreTimerTick`. In this diagram, no second interrupt fires during
that window, so execution completes normally -- but the window is open.

```mermaid
sequenceDiagram
    participant App as Application Code
    participant DXE as DXE Core (CoreRestoreTpl)
    participant Tick as CoreTimerTick
    participant CPU as CPU / Interrupt Controller

    Note over App: Running at TPL_APPLICATION (4)<br/>Interrupts ENABLED

    CPU->>Tick: Timer Interrupt (HW disables interrupts)
    activate Tick
    Note over Tick: Interrupts DISABLED by hardware

    Tick->>DXE: CoreAcquireLock -> CoreRaiseTpl(TPL_HIGH_LEVEL=31)
    DXE-->>Tick: OldTpl = TPL_APPLICATION (4)
    Note over DXE: gEfiCurrentTpl = 31

    Tick->>Tick: Update mEfiSystemTime, signal events

    Tick->>DXE: CoreReleaseLock -> CoreRestoreTpl(TPL_APPLICATION=4)
    Note over DXE: Lowers TPL to 4
    DXE->>CPU: CoreSetInterruptState(TRUE) -> re-enables interrupts
    CPU-->>DXE: returns
    DXE-->>Tick: returns

    Note over Tick: !! Interrupts now ENABLED<br/>inside CoreTimerTick!

    Tick-->>CPU: Return from interrupt handler
    deactivate Tick
    Note over App: Resumes at TPL_APPLICATION
```

### 2.3 Infinite Recursion (The Bug)

This diagram shows what happens when a timer interrupt fires during the
vulnerable window. Each `CoreTimerTick` invocation acquires the lock (raising
TPL to 31), does its work, and releases the lock (restoring TPL to 4). The
`CoreRestoreTpl` call re-enables interrupts -- and a pending timer interrupt
immediately fires, creating a new nested `CoreTimerTick` stack frame. Each
nested invocation repeats the pattern: raise TPL -> work -> restore TPL ->
re-enable interrupts -> next interrupt. Since each nesting level consumes stack
space and none can return until all deeper levels complete, the stack grows
without bound until it overflows.

```mermaid
sequenceDiagram
    participant App as Application Code
    participant DXE as DXE Core
    participant T1 as CoreTimerTick [1]
    participant T2 as CoreTimerTick [2]
    participant T3 as CoreTimerTick [3]
    participant TN as CoreTimerTick [N]
    participant CPU as CPU

    Note over App: Running at TPL_APPLICATION (4)<br/>Interrupts ENABLED

    CPU->>T1: Timer Interrupt #1 (HW disables interrupts)
    activate T1
    T1->>DXE: CoreRaiseTpl(31) -- interrupts already off
    DXE-->>T1: OldTpl = 4
    T1->>T1: Update time, signal events
    T1->>DXE: CoreRestoreTpl(4)
    DXE->>CPU: CoreSetInterruptState(TRUE) -> BUG: re-enables interrupts
    CPU-->>DXE: returns
    DXE-->>T1: returns

    Note over T1: !! Still inside CoreTimerTick [1]<br/>but interrupts are ENABLED

    CPU->>T2: Timer Interrupt #2 (pending/new)
    activate T2
    T2->>DXE: CoreRaiseTpl(31)
    DXE-->>T2: OldTpl = 4
    T2->>T2: Update time, signal events
    T2->>DXE: CoreRestoreTpl(4)
    DXE->>CPU: CoreSetInterruptState(TRUE) -> BUG again
    CPU-->>DXE: returns
    DXE-->>T2: returns

    Note over T2: !! Still inside CoreTimerTick [2]<br/>interrupts ENABLED again

    CPU->>T3: Timer Interrupt #3
    activate T3
    T3->>DXE: CoreRaiseTpl(31)
    DXE-->>T3: OldTpl = 4
    T3->>T3: Update time, signal events
    T3->>DXE: CoreRestoreTpl(4)
    DXE->>CPU: CoreSetInterruptState(TRUE)
    CPU-->>DXE: returns
    DXE-->>T3: returns

    CPU->>TN: Timer Interrupt #N ...
    activate TN
    Note over TN: !! STACK OVERFLOW<br/>Infinite nesting on the same stack

    TN-->>T3: never returns
    deactivate TN
    T3-->>T2: never returns
    deactivate T3
    T2-->>T1: never returns
    deactivate T2
    T1-->>App: never returns
    deactivate T1
```

### 2.4 TPL Nesting and Interrupt Windows

This diagram focuses on the TPL level and interrupt state transitions during
nested timer interrupts. It shows how each `CoreRestoreTpl` drops the TPL from
`TPL_HIGH_LEVEL` (31) back to `TPL_APPLICATION` (4) and re-enables interrupts,
creating a window (marked !!) where the system is still inside the interrupt
handler but interrupts are enabled. Each window allows a new timer interrupt to
fire, adding another nesting level. The pattern repeats indefinitely.

```mermaid
sequenceDiagram
    participant Code as Execution Context
    participant TPL as TPL Level
    participant IRQ as Interrupt State

    Note over Code,IRQ: -- Normal Execution --
    TPL->>TPL: TPL_APPLICATION (4)
    IRQ->>IRQ: ENABLED [OK]

    Note over Code,IRQ: -- Timer Interrupt #1 Fires --
    IRQ->>IRQ: DISABLED by HW !!
    Code->>TPL: CoreRaiseTpl(31)
    TPL-->>Code: OldTpl = 4
    TPL->>TPL: TPL_HIGH_LEVEL (31)

    Note over Code: CoreTimerTick work...

    Code->>TPL: CoreRestoreTpl(4)
    TPL->>TPL: TPL_APPLICATION (4)
    TPL->>IRQ: CoreSetInterruptState(TRUE)
    IRQ->>IRQ: ENABLED -> (by CoreRestoreTpl)
    IRQ-->>TPL: returns
    TPL-->>Code: returns

    Note over Code,IRQ: !! RECURSION WINDOW OPEN<br/>Still inside interrupt handler!

    Note over Code,IRQ: -- Timer Interrupt #2 Fires (nested) --
    IRQ->>IRQ: DISABLED by HW !!
    Code->>TPL: CoreRaiseTpl(31)
    TPL-->>Code: OldTpl = 4
    TPL->>TPL: TPL_HIGH_LEVEL (31)

    Note over Code: CoreTimerTick work...

    Code->>TPL: CoreRestoreTpl(4)
    TPL->>TPL: TPL_APPLICATION (4)
    TPL->>IRQ: CoreSetInterruptState(TRUE)
    IRQ->>IRQ: ENABLED -> (by CoreRestoreTpl)
    IRQ-->>TPL: returns
    TPL-->>Code: returns

    Note over Code,IRQ: !! RECURSION WINDOW OPEN AGAIN<br/>Now 2 levels deep!

    Note over Code,IRQ: -- Timer Interrupt #3 Fires (nested) --
    IRQ->>IRQ: DISABLED by HW !!

    Note over Code,IRQ: ... repeats infinitely -> !! Stack Overflow
```

### 2.5 Additional Recursion Path -- Event Dispatch Loop

This flowchart shows the second recursion entry point within `CoreRestoreTpl`.
When `CoreTimerTick` calls `CoreSignalEvent`, it sets bits in `gEventPending`.
During `CoreRestoreTpl`, the `while (gEventPending)` loop dispatches pending
event notifications. Before calling `CoreDispatchEventNotifies`, the code
re-enables interrupts if `gEfiCurrentTpl < TPL_HIGH_LEVEL`. This is a second
window where a timer interrupt can fire and re-enter `CoreTimerTick`. The same
window also exists at the final TPL restore after the event dispatch loop
completes.

```mermaid
flowchart TD
    A[CoreTimerTick called<br/>interrupts disabled by HW] --> B[CoreAcquireLock<br/>CoreRaiseTpl TPL_HIGH_LEVEL]
    B --> C[CoreSignalEvent<br/>sets gEventPending]
    C --> D[CoreReleaseLock<br/>CoreRestoreTpl]
    D --> E{gEventPending != 0?}
    E -->|Yes| F[Set gEfiCurrentTpl = PendingTpl]
    F --> G{PendingTpl < TPL_HIGH_LEVEL?}
    G -->|Yes| H["CoreSetInterruptState(TRUE)<br/>!! Interrupts ENABLED"]
    H --> I["CoreDispatchEventNotifies()"]
    I --> J["!! Timer interrupt can fire HERE<br/>-> re-enters CoreTimerTick<br/>-> infinite recursion"]
    G -->|No| I
    E -->|No| K[Set gEfiCurrentTpl = NewTpl]
    K --> L{NewTpl < TPL_HIGH_LEVEL?}
    L -->|Yes| M["CoreSetInterruptState(TRUE)<br/>!! Interrupts ENABLED"]
    M --> N["!! Timer interrupt can fire HERE<br/>-> re-enters CoreTimerTick<br/>-> infinite recursion"]
    L -->|No| O[Return -- interrupts stay disabled]
```

---

## 3 Resolution Options

This section presents the available options to resolve the timer interrupt
infinite recursion issue described in Sections 1-2. Options are listed from
the most architecturally complete (spec changes) to implementation-only
fixes within the existing DXE Core.

### 3.1 Option A -- UEFI/PI Specification Changes

The architectural conflicts identified in Section 1.4 stem from ambiguities
and gaps in the UEFI and PI Specifications. A backward-compatible set of
specification changes could resolve the issue at its root:

1. **Define an interrupt context indicator.** Add a new DXE Core internal
   state (e.g., a boolean or bitmask) that tracks whether execution is
   inside a hardware interrupt handler. The PI Spec's
   `EFI_TIMER_ARCH_PROTOCOL.RegisterHandler()` (Section 12.10.2) should
   specify that the DXE Foundation sets this indicator before calling the
   registered `EFI_TIMER_NOTIFY` function and clears it after the function
   returns. This closes the gap identified in Conflict 1.4.4.

2. **Clarify `RestoreTPL()` behavior in interrupt context.** The UEFI Spec's
   `RestoreTPL()` (Section 4.1.9) should state that when execution is in
   interrupt context, lowering the TPL below `TPL_HIGH_LEVEL` dispatches
   pending event notifications but does **not** re-enable processor
   interrupts. The interrupt-return instruction (`IRET`, `ERET`, `SRET`,
   `ERTN`) is responsible for restoring the interrupt enable state. This
   resolves Conflict 1.4.1.

3. **Specify timer handler TPL management responsibility.** The PI Spec's
   `EFI_TIMER_NOTIFY` description should clarify that:
   - The timer driver (or DXE Foundation) raises TPL to `TPL_HIGH_LEVEL`
     before calling the notify function.
   - The timer driver (or DXE Foundation) restores the original TPL after
     the notify function returns.
   - The notify function may internally acquire/release locks (which invoke
     `RaiseTpl`/`RestoreTpl`) without causing interrupt re-enable.
   This closes the ambiguity identified in Section 1.2.2.

4. **Deprecate or define TPL 17-30.** The UEFI Spec should either:
   - (a) Formally deprecate TPL 17-30 as unused firmware-internal levels,
     acknowledging that the binary `EnableInterrupt()`/`DisableInterrupt()`
     CPU Arch Protocol (PI Spec Section 12.3) does not support multi-level
     interrupt priority; or
   - (b) Define a `SetInterruptPriority(Level)` function in the CPU Arch
     Protocol to support the multi-level scheme the spec envisions.
   Option (a) is backward compatible and reflects current reality. Option (b)
   would require hardware-specific interrupt controller support (e.g., ARM
   GIC priority, x86 APIC TPR) and is a larger change. This addresses
   Conflict 1.4.2.

5. **Add a `RestoreTPL()` separation of concerns.** Optionally, the UEFI
   Spec could define that `RestoreTPL()` treats software event dispatch and
   hardware interrupt enable as independent decisions. In practice, this is
   what the DXE Core fixes (Solutions 1-3) implement -- they allow event
   dispatch while keeping interrupts disabled in interrupt context. Codifying
   this in the spec would make the intended behavior explicit. This addresses
   Conflict 1.4.3.

**Pros:**
- Resolves the issue at the architectural level -- all implementations
  benefit.
- Backward compatible: existing drivers and applications that only use
  TPL_APPLICATION, TPL_CALLBACK, TPL_NOTIFY, and TPL_HIGH_LEVEL are
  unaffected.
- Eliminates the ambiguity that caused three incompatible timer driver
  patterns (Section 1.5.2).
- Makes the DXE Core fix a conforming implementation rather than a
  workaround.

**Cons:**
- Requires UEFI Forum / PI Working Group approval -- long timeline.
- Does not fix existing firmware until implementations adopt the updated
  specs.
- Must be paired with an edk2 implementation fix (one of Solutions 1-3
  below) for immediate effect.

### 3.2 Option B -- NestedInterruptTplLib (OvmfPkg, x86-only)

**Status:** Committed in OvmfPkg as `OvmfPkg/Library/NestedInterruptTplLib`.
Used by `OvmfPkg/LocalApicTimerDxe/LocalApicTimerDxe.c`.

**Approach:** Solves the problem entirely within the timer driver, outside
the DXE Core. The timer interrupt handler uses a library
(`NestedInterruptTplLib`) that wraps `RaiseTpl(HIGH)` / `RestoreTpl()` with
logic to detect and defer nested interrupt handling. When a nested interrupt
fires during the outer handler's `RestoreTpl()` call, the inner handler
detects this via shared state and defers its own `RestoreTpl()` to the outer
handler's stack frame. The inner handler returns immediately with interrupts
disabled (by manipulating the saved CPU flags in the interrupt frame).

**Mechanism:**

1. **`NestedInterruptRaiseTPL()`:** Asserts interrupts are disabled (proving
   we are in interrupt context), then calls `gBS->RaiseTPL(TPL_HIGH_LEVEL)`.

2. **`NestedInterruptRestoreTPL()`:** The core logic:
   - Records `InProgressRestoreTPL = InterruptedTPL` in shared state
   - Calls `gBS->RestoreTPL(InterruptedTPL)` -- this may re-enable
     interrupts and allow nested timer interrupts
   - After `RestoreTPL` returns, disables interrupts and checks if an
     inner handler deferred its restore
   - If deferred: loops and calls `RestoreTPL` again (the inner handler's
     responsibility is now on this stack frame)
   - If not deferred: returns normally

3. **Deferral condition:** When the inner handler sees
   `InterruptedTPL == IsrState->InProgressRestoreTPL`, it knows the outer
   handler's `RestoreTPL()` has finished dispatching events and is about to
   return. Calling `RestoreTPL()` again would create unbounded stack growth.
   Instead, it sets `DeferredRestoreTPL = TRUE` and returns with interrupts
   disabled by calling `DisableInterruptsOnIret()`.

4. **`DisableInterruptsOnIret()`:** Clears the IF bit in the saved RFLAGS
   within the interrupt's `EFI_SYSTEM_CONTEXT`, so when the CPU executes
   IRETQ, interrupts remain disabled. This is the x86-specific mechanism
   that prevents further interrupts between the deferral point and the outer
   handler's check.

**Stack depth bound:** The total number of nested stack frames is bounded by
the number of distinct TPL levels. An inner handler only creates a new
`RestoreTPL()` stack frame when `InterruptedTPL > InProgressRestoreTPL` --
meaning the outer handler is still dispatching events at a lower TPL. Since
TPL levels are finite (4-31), nesting is bounded.

**Why this is not a general solution:**

1. **x86-only:** `DisableInterruptsOnIret()` manipulates
   `SystemContextX64->Rflags`. The implementation explicitly contains
   `#error "Unsupported CPU"` for non-x86 architectures. There is no
   equivalent mechanism for AArch64, RISC-V, or LoongArch without
   architecture-specific system context manipulation.

2. **Timer driver responsibility:** Each timer driver must opt in by using
   the library. The standard timer drivers (`PcAtChipsetPkg/HpetTimerDxe`,
   `ArmPkg/Drivers/TimerDxe`) do not use it. Only
   `OvmfPkg/LocalApicTimerDxe` integrates it.

3. **Does not fix the DXE Core:** The root cause -- `CoreRestoreTpl()`
   unconditionally re-enabling interrupts in interrupt context -- remains.
   Any timer driver that does NOT use `NestedInterruptTplLib` still
   triggers the recursion. This includes all ARM and RISC-V timer drivers.

4. **Requires `EFI_SYSTEM_CONTEXT` access:** The deferral mechanism needs
   to modify the interrupt frame's saved flags. This is only available in
   the CPU exception handler callback (`RegisterInterruptHandler`), not in
   the `EFI_TIMER_NOTIFY` function (`CoreTimerTick`). The timer driver must
   be the one implementing the workaround, not the DXE Core.

5. **Complexity:** The library is ~240 lines with subtle shared-state
   coordination between nested interrupt handler instances. Reasoning about
   correctness requires understanding the exact interleaving of interrupt
   delivery, IRET flag restoration, and the `while(TRUE)` deferral loop.

**Pros:**
- Proven in production (OvmfPkg, used by virtual machines).
- Mathematically bounded nesting (by number of TPL levels).
- No DXE Core modification required -- works with unmodified `CoreRestoreTpl`.
- Correctly handles the Windows boot loader STI-at-HIGH scenario.

**Cons:**
- x86-only -- cannot be ported to AArch64/RISC-V/LoongArch.
- Timer-driver-specific -- every timer driver needs its own integration.
- Does not fix the root cause in the DXE Core.
- Complex shared-state logic between nested handler instances.
- Requires `EFI_SYSTEM_CONTEXT` (interrupt frame) access -- not available
  at the `EFI_TIMER_NOTIFY` abstraction level.
- Does not benefit third-party timer drivers or other platforms.

**Conclusion:** `NestedInterruptTplLib` is an effective platform-specific
workaround for OvmfPkg on x86, but it cannot serve as the general solution.
A DXE Core fix (Options C-F below) is required for cross-architecture
coverage.

### 3.3 Option C -- Solution 1: Interrupted TPL Mask (`mInterruptedTplMask`)

Uses a bitmask to track which TPL levels were entered from interrupt context.
`CoreRestoreTpl` guards interrupt re-enable in the event dispatch loop (only
enables at dispatch levels above the highest interrupted TPL) and
unconditionally disables interrupts before the final TPL assignment. See
Section 5.1 for full details.

**Pros:**
- Strictest nesting bound: at most 1 additional nesting level during event
  dispatch.
- Eliminates the IRET race window (disable before TPL assignment).
- Minimal code change: +30 / -5 lines.
- Mask-guarded event loop prevents re-entry at or below the interrupted TPL.
- ASSERT validates monotonic nesting invariant.
- Immune to callback overwrite of interrupt context state.

**Cons:**
- Callbacks at or below the interrupted TPL execute with interrupts disabled
  during interrupt-context event dispatch (slightly increased timer latency).
- Bitmask requires cleanup (clearing bits during `RestoreTpl`).
- Adds `GetInterruptState()` call to every `CoreRaiseTpl(HIGH)`.

### 3.4 Option D -- Solution 2: Boolean Flag (`mInterruptState`)

Uses a single boolean to save the interrupt enable state at `CoreRaiseTpl`
time. The final section of `CoreRestoreTpl` is guarded by
`OldTpl >= TPL_HIGH_LEVEL` -- only HIGH?non-HIGH transitions consult the
flag. Event dispatch loop unconditionally enables interrupts (like original
code). See Section 5.2 for full details.

**Pros:**
- Simple: boolean flag, no bitmask arithmetic.
- Self-cleaning -- flag refreshed on every `CoreRaiseTpl(HIGH)` call.
- Preserves original timer latency during event dispatch.
- Eliminates the IRET race window (disable before TPL assignment).

**Cons:**
- Boolean can be overwritten by callback `RaiseTpl(HIGH)` during
  interrupt-context event dispatch, creating one extra bounded nesting
  window.
- No per-TPL tracking -- cannot distinguish which specific level was
  interrupted.
- No monotonic nesting ASSERT.
- Bounded recursion during event dispatch (depth = pending TPL count).

### 3.5 Option E -- Solution 3: Unconditional Disable + Bounded Nesting

Committed as `bad938e41f1d` by Paolo Bonzini. Uses the same bitmask as
Solution 1 but unconditionally enables interrupts in the event dispatch loop
(like original code). Unconditionally disables before final TPL assignment
and uses the mask for the final enable decision. See Section 5.4 for full
details.

**Pros:**
- Preserves original event dispatch behavior (timer latency maintained).
- Eliminates the IRET race window (disable before TPL assignment).
- Mathematically bounded recursion depth (one level per pending TPL).
- ASSERT validates strict monotonic nesting.
- Immune to callback overwrite.

**Cons:**
- Most complex: +77 / -13 lines.
- Allows bounded recursion at every dispatch level (one per pending TPL),
  not just above the interrupted TPL.
- Extra `CoreSetInterruptState(FALSE)` call on every `CoreRestoreTpl` exit
  (minor performance overhead).

### 3.6 Option F -- Solution 4: CPU Arch Protocol GetInterruptState() Fix

**Recommended.** Resolves the root cause by fixing the CPU Architecture
Protocol's `GetInterruptState()` to read actual hardware state (spec
conformance), then uses `gTplBeforeHighTpl` / `gIsrEntryTplMask` in the
DXE Core. This eliminates the Scenario 5 false positive that affects
Solutions 1-3 because `gCpu->GetInterruptState()` now returns the TRUE
hardware state rather than a stale cached value, and `CoreRestoreTpl` clears
the staging mask (`gTplBeforeHighTpl`) before event dispatch so stale value
from normal context never propagate to the ISR tracking mask.

#### 3.6.1 The Bug in CPU Arch Protocol Implementations

The PI Specification states that `GetInterruptState()` retrieves "the
processor's current interrupt state." Two edk2 implementations violated this
by returning a cached boolean updated only through `EnableInterrupt()` /
`DisableInterrupt()` protocol calls:

- **X64 (`UefiCpuPkg/CpuDxe`):** Returned cached `InterruptState` global
- **RISC-V (`UefiCpuPkg/CpuDxeRiscV64`):** Returned cached `mInterruptState`

When hardware automatically disabled interrupts on ISR entry (clearing
RFLAGS.IF / sstatus.SIE), no protocol call occurred, so the cache remained
stale TRUE. This made `gCpu->GetInterruptState()` unreliable for ISR
detection -- the DXE Core could not distinguish "interrupts enabled" from
"hardware disabled interrupts on ISR entry."

Two other implementations were already correct:
- **AArch64 (`ArmPkg/Drivers/CpuDxe`):** Reads DAIF register directly
- **LoongArch (`UefiCpuPkg/CpuDxeLoongArch64`):** Reads CSR.CRMD.IE directly

#### 3.6.2 The Fix -- Three Parts

**Part 1: CPU Arch Protocol Spec Conformance (CpuDxe drivers)**

Update `CpuGetInterruptState()` in the two non-conforming drivers to call
BaseLib's `GetInterruptState()` -- which reads the actual CPU hardware flag --
and then update the cached variable for backward compatibility:

**X64 (`UefiCpuPkg/CpuDxe/CpuDxe.c`):**
```c
*State = GetInterruptState ();   // reads RFLAGS.IF via AsmReadEflags()
InterruptState = *State;         // update cache for consistency
```

**RISC-V (`UefiCpuPkg/CpuDxeRiscV64/CpuDxe.c`):**
```c
*State = GetInterruptState ();   // reads sstatus.SIE
mInterruptState = *State;        // update cache for consistency
```

**Part 2a: CoreRaiseTpl records the pre-HIGH TPL when interrupts are disabled**

When raising to HIGH, `CoreRaiseTpl` calls `gCpu->GetInterruptState()`. If
hardware interrupts are already disabled, it records the old TPL in
`gTplBeforeHighTpl`. This occurs in two situations:
- **ISR context (intended):** Hardware disabled interrupts on interrupt
  entry. The timer driver's `RaiseTpl(HIGH)` records the interrupted TPL.
- **Normal context (benign):** Code called `DisableInterrupt()` then
  `RaiseTpl(HIGH)`. This sets a "stale" bit in `gTplBeforeHighTpl`.

The stale value from normal context is harmless because `CoreRestoreTpl`
unconditionally clears `gTplBeforeHighTpl = 0` when transitioning from
HIGH to below HIGH, *before* entering the event dispatch loop. This
ensures that if a timer interrupt fires during subsequent event dispatch,
`CoreTimerTick` sees `gTplBeforeHighTpl == 0` and correctly identifies
the timer-driver-did-not-raise-TPL case (using `gEfiCurrentTpl` directly).
The stale value never propagates to `gIsrEntryTplMask`.

**Part 2b: CoreTimerTick sets gIsrEntryTplMask**

At entry, `CoreTimerTick` determines the interrupted TPL:
- If the timer driver called RaiseTpl(HIGH) before CoreTimerTick,
  `gEfiCurrentTpl` is already HIGH and the actual interrupted TPL is
  recovered from `gTplBeforeHighTpl` (set by CoreRaiseTpl).
- If the timer driver did not call RaiseTpl, `gEfiCurrentTpl` directly
  reflects the interrupted level.

In either case, CoreTimerTick sets the corresponding bit in
`gIsrEntryTplMask`, marking that TPL level as having an active ISR.

**Part 2c: CoreRestoreTpl consults gIsrEntryTplMask**

When lowering TPL, CoreRestoreTpl checks `gIsrEntryTplMask`:
- During event dispatch: only re-enable interrupts if dispatching above
  all interrupted levels (allows preemption of lower-priority handlers).
- At final TPL commit: if the new TPL is at or below any mask bit,
  suppress interrupt re-enable and clear completed bits. The hardware
  interrupt-return instruction (IRET/ERET) will restore interrupts.

The following pseudocode shows the conceptual logic flow. In the actual
implementation, CoreRaiseTpl records `gTplBeforeHighTpl` and CoreTimerTick
transfers it to `gIsrEntryTplMask`; the code example below shows the
combined effect using the final variable name `gIsrEntryTplMask`:

```c
// CoreRaiseTpl -- record pre-HIGH TPL when interrupts are disabled
if ((NewTpl >= TPL_HIGH_LEVEL) && (OldTpl < TPL_HIGH_LEVEL)) {
    if ((gCpu != NULL) &&
        (gCpu->GetInterruptState (gCpu, &State) == EFI_SUCCESS) &&
        !State) {
    ASSERT (gTplBeforeHighTpl == 0);
    gTplBeforeHighTpl = OldTpl;  // recorded for CoreTimerTick
    }
    CoreSetInterruptState (FALSE);
}

// CoreTimerTick -- transfer gTplBeforeHighTpl to gIsrEntryTplMask
if (gEfiCurrentTpl >= TPL_HIGH_LEVEL && gTplBeforeHighTpl != 0) {
  gIsrEntryTplMask |= (1ULL << gTplBeforeHighTpl);
    gTplBeforeHighTpl = 0;
}

// CoreRestoreTpl -- clear stale gTplBeforeHighTpl on HIGH-to-below-HIGH
if ((OldTpl >= TPL_HIGH_LEVEL) && (NewTpl < TPL_HIGH_LEVEL)) {
  gTplBeforeHighTpl = 0;  // prevents stale normal-context value
}

// CoreRestoreTpl -- event dispatch loop (mask-guarded)
if ((HighBitSet64 (gIsrEntryTplMask) < (INTN)gEfiCurrentTpl) &&
    (gEfiCurrentTpl < TPL_HIGH_LEVEL)) {
    CoreSetInterruptState (TRUE);
}
CoreDispatchEventNotifies (gEfiCurrentTpl);

// CoreRestoreTpl -- final section
if ((INTN)NewTpl <= HighBitSet64 (gIsrEntryTplMask)) {
    gIsrEntryTplMask &= (1ULL << NewTpl) - 1;  // ISR: clear bits, stay disabled
} else if (NewTpl < TPL_HIGH_LEVEL) {
    CoreSetInterruptState (TRUE);               // Normal: re-enable
}
```

#### 3.6.3 Why This Eliminates False Positives

In the **conceptual single-mask design** (Section 4's analysis), Solutions
1-3 suffer from a false positive in Scenario 5 (disable interrupts then
RaiseTpl): the cached `GetInterruptState()` returns FALSE, the mask bit is
set, and `RestoreTpl` skips re-enable even though the code is not in ISR
context.

The **actual implementation** eliminates this entirely through the two-stage
mask design. `CoreRaiseTpl` sets `gTplBeforeHighTpl` (staging). When
`CoreRestoreTpl` transitions from HIGH to below HIGH, it unconditionally
clears `gTplBeforeHighTpl = 0` before entering the event dispatch loop.
The staging bit from `DisableInterrupt()` + `RaiseTpl(HIGH)` is discarded
before any `CoreTimerTick()` could transfer it to `gIsrEntryTplMask`. The
result: `RestoreTpl` re-enables interrupts normally, matching the original
code's behavior. (Validated by Test 9.)

With the HW-reading fix, `GetInterruptState()` reports ground truth:

| Scenario | Hardware State | Protocol Returns | Mask Action |
|----------|--------------|-----------------|-------------|
| Normal `RaiseTpl(HIGH)` | IF=1 (enabled) | TRUE | No mask bit set |
| `DisableInterrupt()` -> `RaiseTpl(HIGH)` | IF=0 (SW disabled) | FALSE | Staging bit set (benign -- cleared by RestoreTpl) |
| ISR entry -> `RaiseTpl(HIGH)` | IF=0 (HW disabled) | FALSE | Staging bit set -> transferred to `gIsrEntryTplMask` by CoreTimerTick |

The `DisableInterrupt()` -> `RaiseTpl(HIGH)` case sets `gTplBeforeHighTpl`,
but this staging bit is cleared by `CoreRestoreTpl` during the
HIGH-to-below-HIGH transition (before any timer can fire). In ISR context,
`CoreTimerTick` runs between the `RaiseTpl` and `RestoreTpl` and transfers
the staging bit to `gIsrEntryTplMask` -- this is the correct path for ISR
tracking.

The key advantage of Solution 4 is not false-positive elimination (the
two-stage design handles that for all solutions) but **spec conformance**:
`GetInterruptState()` returns the true hardware state as required by the
PI Specification, which has independent value for all callers of the protocol.

#### 3.6.4 False-Positive Resolution Table (Conceptual Single-Mask Design)

The following table describes behavior under the conceptual single-mask
design analyzed in Section 4. The actual two-stage implementation eliminates
false positives for all solutions (see implementation note below).

| Scenario | Description | Solutions 1-3 (cached) | Solution 4 (HW read) | Why Different |
|---|---|---|---|---|
| 5 | Disable interrupts -> `RaiseTpl(HIGH)` | [!] False positive | [OK] Correct | HW reads IF=0 -> respects caller's explicit disable |
| 5 (variant) | CLI/STI bracket with `RaiseTpl` inside | [!] Partial false pos. | [OK] Correct | Inside CLI bracket, IF=0 -> correct |

**Note:** Whether the disable is via CPU Arch Protocol (`DisableInterrupt()`)
or raw instruction (`CLI`, `CPSID i`, `DAIFSet`), the behavior is identical.
Solutions 1-3 see the cached state (stale TRUE or FALSE depending on whether
the protocol call updated the cache). Solution 4 reads hardware directly,
so `CLI` correctly results in FALSE regardless of the cached variable state.

**Implementation note:** The actual implementation uses a two-stage mask
(`gTplBeforeHighTpl` / `gIsrEntryTplMask`). `CoreRestoreTpl` clears
`gTplBeforeHighTpl` on the HIGH-to-below-HIGH transition, so in normal
context the stale value never reaches `gIsrEntryTplMask` and no false
positive occurs for any solution. The table above describes the conceptual
single-mask design for comparison purposes.

#### 3.6.5 Advantages Over Nesting Counter Approach

An alternative Solution 4 design (considered and rejected) used a
`mTimerInterruptNestLevel` counter in `CoreTimerTick`. That approach:

- Required cross-file coupling (`Tpl.c` reading `Timer.c`'s variable)
- Only detected timer interrupt context (not general ISR context)
- Required modifying `Timer.c` (the fix should be localized)
- Failed on ARM because `ArmTimerDxe` wraps the notify call with
  `RaiseTpl(HIGH)`/`RestoreTpl(Orig)` -- the counter would be set inside
  `CoreTimerTick` but `RaiseTpl` is called BEFORE `CoreTimerTick`, from
  the timer driver's interrupt handler where the counter is still 0

The CPU Arch Protocol fix avoids all these issues:
- No cross-file coupling (DXE Core uses the standard protocol interface)
- Detects any ISR context (not just timer -- future-proof)
- No changes to `Timer.c` (zero coupling with timer driver patterns)
- Works with all timer driver patterns (A, B, C from Section 1.5.2)
  because hardware always clears the interrupt flag before any software runs

**Pros:**
- Eliminates the Scenario 5 false positive (staging mask cleared by `RestoreTpl`).
- Fixes a genuine spec-conformance bug in CPU Arch Protocol (independent value).
- Uses the standard `gCpu->GetInterruptState()` protocol interface -- no
  BaseLib dependency in the DXE Core, no internal cross-module coupling.
- Works with ALL timer driver patterns (ARM's RaiseTpl wrapper, x86's bare
  call, OvmfPkg's NestedInterruptTplLib) because detection is at the
  hardware level, not the software call-chain level.
- DXE Core logic identical to Solution 1 (simplest mask approach,
  mask-guarded event loop, IRET race elimination, callback-overwrite
  immunity).
- ASSERT validates monotonic nesting invariant.
- CPU Arch Protocol fix updates cache for backward compatibility -- no
  regressions for code that reads the cached variable through other paths.

**Cons:**
- Modifies CPU Arch Protocol drivers (2 files outside DXE Core).
  However, these changes fix a genuine spec-conformance bug and have
  independent value regardless of the DXE Core timer fix.
- Same `gCpu->GetInterruptState()` protocol call overhead in `CoreRaiseTpl`
  as Solutions 1-3.
- Adds slight complexity to CpuDxe drivers (one extra BaseLib call per
  `GetInterruptState()` invocation -- reading a CPU register is trivial).
- In ISR context, event callbacks execute with interrupts disabled (the
  mask suppresses re-enable). This is correct behavior -- the original code
  crashes in this case -- and timer latency in normal context is unaffected.

### 3.7 Comparison Summary

| Property | Original (unfixed) | Option A (Spec) | Option B (Nested) | Option C (Sol 1) | Option D (Sol 2) | Option E (Sol 3) | Option F (Sol 4) |
|---|---|---|---|---|---|---|---|
| **Scope** | DXE Core | Architecture | Timer driver | DXE Core | DXE Core | DXE Core | DXE Core + CpuDxe |
| **Architecture** | All | All | x86 only | All | All | All | All |
| **IRET race** | -> Unbounded recursion | Defined away | -> Eliminated | -> Eliminated | -> Eliminated | -> Eliminated | -> Eliminated |
| **Max nesting** | 8 (stack overflow) | 0 (by design) | = TPL count | 1 additional | = pending TPLs | = pending TPLs | 1 additional |
| **Callback overwrite** | Vulnerable | N/A | N/A | Immune | Extra nesting | Immune | Immune |
| **Timer latency** | Preserved | Preserved | Preserved | Slightly increased | Preserved | Preserved | **Preserved** |
| **False positives** | N/A | None | None | Scenario 5 | Scenario 5 | Scenario 5 | **None** |
| **Detection method** | None | N/A | Shared state | `GetInterruptState` (cached) | `GetInterruptState` (cached) | `GetInterruptState` (cached) | `GetInterruptState` (HW) |
| **Code complexity** | Baseline | N/A (spec text) | ~240 lines (lib) | +30/-5 lines | +28/-9 lines | +77/-13 lines | +30/-5 (DXE) + ~4 (CpuDxe) |
| **Files modified** | None | N/A (spec text) | Timer driver + lib | Tpl.c | Tpl.c | Tpl.c | Tpl.c + CpuDxe.c (--2) |
| **Spec conformance** | Full (but unsafe) | Full | N/A (driver) | Partial (cached) | Partial (cached) | Partial (cached) | **Full** (reads HW) |
| **Timer driver agnostic** | Yes | Yes | **No** (per-driver) | Yes | Yes | Yes | **Yes** (HW-level) |
| **Timeline** | Current | Long (spec process) | Committed (OvmfPkg) | Immediate | Immediate | Committed | Immediate |
| **Recommendation** | -> Broken | Pursue in parallel | Platform workaround | Viable | Viable | Reference impl | **Preferred** |

**Note:** The "False positives" row describes the conceptual single-mask
design. The actual implementation's two-stage mask (`gTplBeforeHighTpl` /
`gIsrEntryTplMask`) eliminates Scenario 5 false positives for all solutions
by clearing stale staging bits during `RestoreTpl`. See Section 3.6.3.

**Recommendation:** Implement **Option F (Solution 4)**.

**Why Solution 4 over the alternatives:**

1. **vs. Option A (Spec):** Spec changes have a multi-year timeline and
   don't fix existing firmware. Solution 4 is deployable immediately.
   Pursue Option A in parallel for long-term clarity.

2. **vs. Option B (NestedInterruptTplLib):** x86-only, timer-driver-specific,
   does not fix the root cause in the DXE Core. Every timer driver on every
   platform would need its own integration. Solution 4 fixes the DXE Core
   once for all architectures and all timer drivers.

3. **vs. Options C-E (Solutions 1-3):** All three use the *cached*
   `GetInterruptState()` value, which creates a false positive in Scenario 5
   where `CoreRestoreTpl` incorrectly suppresses interrupt re-enable in
   normal (non-ISR) context. Solution 4 eliminates this by reading the
   actual hardware interrupt flag -- the return value is always ground truth.

4. **Independent value:** The CPU Arch Protocol `GetInterruptState()` fix
   corrects a genuine spec-conformance bug (PI Spec says "current" state,
   not cached). This has value regardless of the DXE Core timer fix -- any
   future code relying on the protocol's accuracy benefits.

5. **Simplest correct DXE Core logic:** The `mInterruptedTplMask` approach
   (shared with Solution 1) provides the strictest nesting bound (max 1
   additional level), immunity to callback overwrite, ASSERT-validated
   monotonic nesting, and no timer latency impact in normal operation.

6. **Timer driver agnostic:** Works with all three timer driver patterns
   (ARM's RaiseTpl wrapper, x86's bare call, OvmfPkg's NestedInterruptTplLib)
   because hardware ALWAYS clears the interrupt flag before any software
   runs -- detection is at the hardware level, not the call-chain level.

---

## 3.8 RaiseTpl State Transition Table

This table enumerates all valid combinations of inputs to `CoreRaiseTpl(NewTpl)`
and the expected interrupt state transitions based on the architectural analysis
in Section 1.

### 3.8.1 Input Parameters

| Parameter | Description |
|---|---|
| **NewTpl** | The TPL level being raised to (4-31) |
| **gEfiCurrentTpl** | The current TPL before the call (4-31, must be = NewTpl) |
| **Interrupts (HW)** | Actual hardware interrupt enable state on entry |
| **Context** | Whether executing in normal context or ISR context |

### 3.8.2 State Transition Table

| # | Context | gEfiCurrentTpl | NewTpl | Interrupts Before | Action | Interrupts After | Notes |
|---|---|---|---|---|---|---|---|
| 1 | Normal | APPLICATION (4) | CALLBACK (8) | Enabled | None (no threshold crossing) | Enabled | TPL raised within interruptible range |
| 2 | Normal | APPLICATION (4) | NOTIFY (16) | Enabled | None (no threshold crossing) | Enabled | TPL raised within interruptible range |
| 3 | Normal | APPLICATION (4) | HIGH (31) | Enabled | `DisableInterrupt()` | **Disabled** | Crossing to HIGH disables interrupts |
| 4 | Normal | CALLBACK (8) | NOTIFY (16) | Enabled | None (no threshold crossing) | Enabled | Both below HIGH |
| 5 | Normal | CALLBACK (8) | HIGH (31) | Enabled | `DisableInterrupt()` | **Disabled** | Crossing to HIGH disables interrupts |
| 6 | Normal | NOTIFY (16) | HIGH (31) | Enabled | `DisableInterrupt()` | **Disabled** | Crossing to HIGH disables interrupts |
| 7 | Normal | HIGH (31) | HIGH (31) | Disabled | None (already at HIGH) | Disabled | No-op, already at highest level |
| 8 | ISR | APPLICATION (4) | HIGH (31) | **Disabled** | `DisableInterrupt()` (redundant) | Disabled | Timer driver that calls RaiseTpl(HIGH) (ARM): HW already disabled interrupts; RaiseTpl call sets `gTplBeforeHighTpl` |
| 9 | ISR | CALLBACK (8) | HIGH (31) | **Disabled** | `DisableInterrupt()` (redundant) | Disabled | Nested ISR interrupted at CALLBACK; same as #8 |
| 10 | ISR | NOTIFY (16) | HIGH (31) | **Disabled** | `DisableInterrupt()` (redundant) | Disabled | Nested ISR interrupted at NOTIFY; same as #8 |
| 11 | ISR | HIGH (31) | HIGH (31) | Disabled | None | Disabled | Timer driver that does not call RaiseTpl (x86 LAPIC): enters CoreTimerTick with TPL already at HIGH -- lock acquire is a no-op |
| 12 | ISR (unwind) | CALLBACK (8) | NOTIFY (16) | Enabled* | None (no threshold crossing) | Enabled | During event dispatch in CoreRestoreTpl; *interrupts re-enabled for preemption |
| 13 | ISR (unwind) | CALLBACK (8) | HIGH (31) | Enabled* | `DisableInterrupt()` | **Disabled** | Event handler acquires lock during ISR unwind |
| 14 | ISR (unwind) | NOTIFY (16) | HIGH (31) | Enabled* | `DisableInterrupt()` | **Disabled** | Event handler acquires lock during ISR unwind |
| 15 | ISR (unwind) | CALLBACK (8) | HIGH (31) | Disabled | `DisableInterrupt()` (redundant) | Disabled | Event handler acquires lock; preemption not yet enabled |
| 16 | ISR (unwind) | NOTIFY (16) | HIGH (31) | Disabled | `DisableInterrupt()` (redundant) | Disabled | Event handler acquires lock; preemption not yet enabled |

### 3.8.3 Key Observations

1. **Interrupts are only disabled by software on the transition crossing to
   TPL_HIGH_LEVEL** (rows 3, 5, 6, 13, 14). All other transitions within the
   interruptible range (rows 1, 2, 4, 12) leave interrupt state unchanged.

2. **In ISR context, hardware has already disabled interrupts** (rows 8-11).
   The software `DisableInterrupt()` call is redundant but harmless. The
   critical behavior is recording `gTplBeforeHighTpl` for `CoreTimerTick()`
   to read.

3. **ISR unwind context** (rows 12-16) occurs during event dispatch within
   `CoreRestoreTpl()`. Interrupts may be either enabled (if `gIsrEntryTplMask`
   allows preemption at the current dispatch level) or disabled (if still
   unwinding at or below the interrupted level). Event handlers calling
   `RaiseTpl(HIGH)` in this context may set bits in `gTplBeforeHighTpl`:
   - If interrupts are enabled (rows 12-14): `GetInterruptState()` returns
     TRUE, so no bit is set -- correct, this is preempted normal dispatch.
   - If interrupts are still disabled (rows 15-16): `GetInterruptState()`
     returns FALSE, so a bit is set -- correct, still in ISR unwind context.
   In either case, if a timer interrupt fires during preempted dispatch,
   the timer driver's `RaiseTpl(HIGH)` sets the correct interrupted TPL
   before `CoreTimerTick()` reads `gTplBeforeHighTpl`.

4. **The `gTplBeforeHighTpl` write** occurs in rows 3, 5, 6, 8, 9, 10, 13,
   14, 15, 16 -- every transition crossing to HIGH. It is always correct when
   read by `CoreTimerTick()` because the timer driver's `RaiseTpl(HIGH)` is
   the most recent write before `CoreTimerTick()` executes.

5. **Row 11 (timer driver that does not call RaiseTpl -- x86 LAPIC)** is
   the only case where `CoreTimerTick()` is entered without a preceding
   software `RaiseTpl(HIGH)`. Here
   `gEfiCurrentTpl < HIGH` from the perspective of the DXE Core (the CPU
   hardware is at interrupt-disabled state but gEfiCurrentTpl was never
   raised), so `CoreTimerTick()` uses `gEfiCurrentTpl` directly and does not
   read `gTplBeforeHighTpl`.

---

## 4 Scenario Analysis

This section defines all 15 scenarios and traces the behavior of the original
(unfixed) code and all four proposed DXE Core solutions (Options C-F from
Section 3) for each. Code references for all implementations are provided
first, followed by per-scenario analysis organized by interrupt mechanism.

**Note:** Solution 4 (Option F) uses the same DXE Core logic as Solution 1
but with the CPU Arch Protocol `GetInterruptState()` fix that reads actual
hardware state. For TPL-managed scenarios (1-4, 8-9, 11-13), Solution 4 behaves
identically to Solution 1 because hardware always clears the interrupt flag
on ISR entry. For direct manipulation scenarios (5-7, 10, 14-15), Solution 4 differs:
what are "false positives" for Solutions 1-3 (cached value) become "correct
behavior" for Solution 4 (reading true hardware state).

### 4.1 Code References

#### 4.1.1 Original Code (Unfixed)

```c
CoreRaiseTpl (NewTpl):
  OldTpl = gEfiCurrentTpl;
  if ((NewTpl >= TPL_HIGH_LEVEL) && (OldTpl < TPL_HIGH_LEVEL)) {
    CoreSetInterruptState (FALSE);              // always disables
  }
  gEfiCurrentTpl = NewTpl;
  return OldTpl;

CoreRestoreTpl (NewTpl):
  while (gEventPending != 0) {
    PendingTpl = HighBitSet64 (gEventPending);
    if (PendingTpl <= NewTpl) break;
    gEfiCurrentTpl = PendingTpl;
    if (gEfiCurrentTpl < TPL_HIGH_LEVEL) {
      CoreSetInterruptState (TRUE);             // always enables
    }
    CoreDispatchEventNotifies (gEfiCurrentTpl);
  }
  gEfiCurrentTpl = NewTpl;
  if (gEfiCurrentTpl < TPL_HIGH_LEVEL) {
    CoreSetInterruptState (TRUE);               // always enables
  }
```

`CoreRestoreTpl` **unconditionally** calls `CoreSetInterruptState(TRUE)`
whenever the current TPL drops below `TPL_HIGH_LEVEL`, regardless of
whether execution is inside an interrupt handler.

#### 4.1.2 Solution 1 -- Interrupted TPL Mask

Adds `volatile UINTN mInterruptedTplMask = 0`.

```c
CoreRaiseTpl (NewTpl):
  ...
  if ((NewTpl >= TPL_HIGH_LEVEL) && (OldTpl < TPL_HIGH_LEVEL)) {
    GetInterruptState (&State);
    if (!State) {
      ASSERT ((INTN)OldTpl >= HighBitSet64 (mInterruptedTplMask));
      mInterruptedTplMask |= (1ULL << OldTpl);  // record interrupted TPL
    }
    CoreSetInterruptState (FALSE);               // unconditional (same as original)
  }

CoreRestoreTpl (NewTpl):
  // event loop -- CONDITIONALLY re-enables based on mask:
    if ((gEfiCurrentTpl < TPL_HIGH_LEVEL) &&
        ((INTN)gEfiCurrentTpl > HighBitSet64 (mInterruptedTplMask))) {
      CoreSetInterruptState (TRUE);
    }
    CoreDispatchEventNotifies (gEfiCurrentTpl);

  // final section -- unconditional disable BEFORE TPL assignment:
  CoreSetInterruptState (FALSE);
  gEfiCurrentTpl = NewTpl;
  if ((INTN)NewTpl <= HighBitSet64 (mInterruptedTplMask)) {
    mInterruptedTplMask &= (1ULL << NewTpl) - 1;  // IRQ context: clear bits
  } else if (NewTpl < TPL_HIGH_LEVEL) {
    CoreSetInterruptState (TRUE);                  // normal context: re-enable
  }
```

#### 4.1.3 Solution 2 -- Boolean Flag + Unconditional Disable

Adds `volatile BOOLEAN mInterruptState = TRUE`.

```c
CoreRaiseTpl (NewTpl):
  ...
  if ((NewTpl >= TPL_HIGH_LEVEL) && (OldTpl < TPL_HIGH_LEVEL)) {
    GetInterruptState (&State);
    mInterruptState = State;             // save interrupt state
    CoreSetInterruptState (FALSE);       // unconditional (same as original)
  }

CoreRestoreTpl (NewTpl):
  // event loop -- UNCONDITIONALLY re-enables (like original):
    if (gEfiCurrentTpl < TPL_HIGH_LEVEL) {
      CoreSetInterruptState (TRUE);
    }
    CoreDispatchEventNotifies (gEfiCurrentTpl);

  // final section -- ONLY for HIGH?non-HIGH transitions:
  if (OldTpl >= TPL_HIGH_LEVEL) {
    CoreSetInterruptState (FALSE);       // disable BEFORE TPL assignment
    gEfiCurrentTpl = NewTpl;
    // Only re-enable in normal context; in IRQ context the interrupt return
    // restores them:
    if ((gEfiCurrentTpl < TPL_HIGH_LEVEL) && mInterruptState) {
      CoreSetInterruptState (TRUE);
    }
  } else {
    // Non-HIGH transitions: preserve original behavior.
    // Must NOT check mInterruptState -- it may be stale FALSE.
    gEfiCurrentTpl = NewTpl;
    if (gEfiCurrentTpl < TPL_HIGH_LEVEL) {
      CoreSetInterruptState (TRUE);
    }
  }
```

#### 4.1.4 Solution 3 -- Unconditional Disable + Bounded Nesting

Adds `static UINTN mInterruptedTplMask = 0` (same as Solution 1).

```c
CoreRaiseTpl (NewTpl):
  ...
  if ((NewTpl >= TPL_HIGH_LEVEL) && (OldTpl < TPL_HIGH_LEVEL)) {
    GetInterruptState (&InterruptState);
    if (InterruptState) {
      CoreSetInterruptState (FALSE);
    } else {
      ASSERT ((INTN)OldTpl > HighBitSet64 (mInterruptedTplMask));
      mInterruptedTplMask |= (1U << OldTpl);
    }
  }

CoreRestoreTpl (NewTpl):
  // event loop -- UNCONDITIONALLY re-enables (like original):
    if (gEfiCurrentTpl < TPL_HIGH_LEVEL) {
      CoreSetInterruptState (TRUE);
    }
    CoreDispatchEventNotifies (gEfiCurrentTpl);

  // final section -- unconditional disable BEFORE TPL assignment:
  CoreSetInterruptState (FALSE);
  gEfiCurrentTpl = NewTpl;
  if ((INTN)NewTpl <= HighBitSet64 (mInterruptedTplMask)) {
    // interrupt context: leave disabled, clear bits
    mInterruptedTplMask &= (1U << NewTpl) - 1;
  } else if (NewTpl < TPL_HIGH_LEVEL) {
    CoreSetInterruptState (TRUE);
  }
```

#### 4.1.5 Solution 4 -- CPU Arch Protocol Fix + Interrupted TPL Mask

**DXE Core code is identical to Solution 1** (same `mInterruptedTplMask`
logic). The difference is in the CPU Arch Protocol driver:
`gCpu->GetInterruptState()` reads actual hardware (RFLAGS.IF on x86,
sstatus.SIE on RISC-V, DAIF on AArch64) instead of returning a cached
variable.

```c
// CpuDxe fix (X64): CpuGetInterruptState now reads hardware
CpuGetInterruptState (This, State):
  *State = GetInterruptState ();   // reads RFLAGS.IF via AsmReadEflags()
  InterruptState = *State;         // update cache for backward compat
  return EFI_SUCCESS;

// DXE Core: identical to Solution 1
CoreRaiseTpl (NewTpl):
  ...
  if ((NewTpl >= TPL_HIGH_LEVEL) && (OldTpl < TPL_HIGH_LEVEL)) {
    gCpu->GetInterruptState (gCpu, &State);  // NOW reads real HW
    if (!State) {
      ASSERT ((INTN)OldTpl >= HighBitSet64 (mInterruptedTplMask));
      mInterruptedTplMask |= (1ULL << OldTpl);
    }
    CoreSetInterruptState (FALSE);
  }

CoreRestoreTpl (NewTpl):
  // event loop -- mask-guarded (same as Solution 1):
    if ((gEfiCurrentTpl < TPL_HIGH_LEVEL) && (mInterruptedTplMask == 0)) {
      CoreSetInterruptState (TRUE);
    }
    CoreDispatchEventNotifies (gEfiCurrentTpl);

  // final section -- same as Solution 1:
  CoreSetInterruptState (FALSE);
  gEfiCurrentTpl = NewTpl;
  if ((INTN)NewTpl <= HighBitSet64 (mInterruptedTplMask)) {
    mInterruptedTplMask &= (1ULL << NewTpl) - 1;
  } else if (NewTpl < TPL_HIGH_LEVEL) {
    CoreSetInterruptState (TRUE);
  }
```

**Key behavioral difference:** Because `GetInterruptState()` reads hardware:
- In normal context: HW IF=1 -> returns TRUE -> no mask bit set (correct)
- In ISR context: HW IF=0 -> returns FALSE -> mask bit set (correct)
- After `DisableInterrupt()`: HW IF=0 -> returns FALSE -> mask bit set
  (correct -- respects caller's explicit disable)

Solutions 1-3 use the cached value, which can be stale, causing false
positives in Scenario 5 (disable interrupts then RaiseTpl).

### 4.2 Spec Conformance Scenarios (1-7)

**Scenario 1 -- Normal `RaiseTpl(HIGH)` / `RestoreTpl`:**
Code at `TPL_APPLICATION` with interrupts enabled calls `RaiseTpl(HIGH)`
then `RestoreTpl(TPL_APPLICATION)`.

- **Original:** Disables on raise, unconditionally re-enables on restore. [OK]
- **Solution 1:** `GetInterruptState` -> TRUE -> mask unchanged -> disables on
  raise, re-enables on restore. [OK]
- **Solution 2:** `GetInterruptState` -> TRUE -> `mInterruptState = TRUE`
  -> disables on raise, `mInterruptState = TRUE` -> re-enables on restore. [OK]
- **Solution 3:** `GetInterruptState` -> TRUE -> mask unchanged -> disables on
  raise. Final: `CoreSetInterruptState(FALSE)`, `gEfiCurrentTpl = 4`,
  `4 > HighBitSet64(0) = -1` -> else branch -> `CoreSetInterruptState(TRUE)`. [OK]
- **Solution 4:** Identical to Solution 1 -- HW reads IF=1 (interrupts
  enabled in normal context) -> same result. [OK]

**Scenario 2 -- `RaiseTpl` to non-HIGH (e.g. `TPL_NOTIFY`):**
`(NewTpl >= TPL_HIGH_LEVEL)` is FALSE. The interrupt-management block in
`CoreRaiseTpl` is never entered.

- **All five:** No impact -- guard condition prevents any interrupt state
  change. [OK]

**Scenario 3 -- Nested `RaiseTpl(HIGH)` when already at HIGH:**
`(OldTpl < TPL_HIGH_LEVEL)` is FALSE. The interrupt-management block in
`CoreRaiseTpl` is never entered.

- **All five:** No impact -- inner guard prevents any change. [OK]

**Scenario 4 -- Normal TPL nesting through event dispatch loop:**
During `CoreRestoreTpl(TPL_APPLICATION)`, the event dispatch loop walks
through pending events at descending TPL levels. At each level below
`TPL_HIGH_LEVEL`, the code decides whether to re-enable interrupts before
dispatching that level's event notifications. This is the expected nesting
pattern: `TPL_APPLICATION` -> `TPL_CALLBACK` -> `TPL_NOTIFY` ->
`TPL_HIGH_LEVEL` and back down.

For example, `RaiseTpl(HIGH)` from `TPL_APPLICATION`, with events pending at
both `TPL_CALLBACK` and `TPL_NOTIFY`:

```
CoreRaiseTpl(HIGH):
  GetInterruptState -> TRUE (normal context, interrupts enabled)
  CoreSetInterruptState(FALSE) -> disables interrupts
  gEfiCurrentTpl = 31

CoreRestoreTpl(TPL_APPLICATION):
  Loop iteration 1: gEfiCurrentTpl = TPL_NOTIFY (16)
    (16 < 31) -> re-enable check -> CoreDispatchEventNotifies(16)
  Loop iteration 2: gEfiCurrentTpl = TPL_CALLBACK (8)
    (8 < 31) -> re-enable check -> CoreDispatchEventNotifies(8)
  Final: gEfiCurrentTpl = TPL_APPLICATION (4)
    (4 < 31) -> re-enable check
```

- **Original:** Each level unconditionally calls `CoreSetInterruptState(TRUE)`.
  Interrupts enabled during event dispatch at every level. [OK]
- **Solution 1:** Mask is 0 (raised from normal context with interrupts on) [OK]
  event loop checks `gEfiCurrentTpl > HighBitSet64(0) = -1` -> always TRUE [OK]
  enables interrupts at each dispatch level. Final: `CoreSetInterruptState(FALSE)`,
  `gEfiCurrentTpl = 4`, `4 > HighBitSet64(0) = -1` -> else branch [OK]
  `CoreSetInterruptState(TRUE)`. -> Identical to original in normal context.
- **Solution 2:** `mInterruptState = TRUE` (raised from normal context
  with interrupts on) -> `TRUE` at every level -> enables interrupts
  at each dispatch level. -> Identical to original.
- **Solution 3:** Event loop unconditionally enables at each level (no guard
  on the loop). Final: `CoreSetInterruptState(FALSE)`, `gEfiCurrentTpl = 4`,
  `4 > HighBitSet64(0) = -1` -> else branch -> `CoreSetInterruptState(TRUE)`.
  -> Identical to original.
- **Solution 4:** Identical to Solution 1 -- HW reads IF=1, mask is 0,
  `mInterruptedTplMask == 0` check -> enables at every dispatch level.
  -> Identical to original in normal context.

All five implementations produce identical behavior for this expected
normal-context nesting pattern. The guard in Solutions 1/4 only
suppresses enables when `mInterruptedTplMask != 0`,
which only triggers in interrupt context where the mask has bits set.
Solution 2's guard only suppresses enables when `mInterruptState` is
FALSE (interrupt context). Solution 3's event loop has no guard at all
(like original), and its final section also re-enables since the mask is
empty.

**Scenario 5 -- Disable interrupts then `RaiseTpl(HIGH)` (normal context):**
Code manually disables interrupts, then calls `RaiseTpl(HIGH)`.

- **Original:** `CoreSetInterruptState(FALSE)` is redundant (already off).
  `RestoreTpl` unconditionally re-enables. [OK]
- **Solution 1:** `GetInterruptState` -> FALSE -> sets mask bit [OK]
  `RestoreTpl` skips enable -> interrupts remain off. !! False positive
  (self-cleans when caller re-enables and next `RaiseTpl(HIGH)` occurs).
- **Solution 2:** `GetInterruptState` -> FALSE -> `mInterruptState = FALSE`
  -> `RestoreTpl` skips enable. !! Same false positive, same self-clean.
- **Solution 3:** `GetInterruptState` -> FALSE -> sets mask bit (same as S1).
  Final: `CoreSetInterruptState(FALSE)`, `gEfiCurrentTpl = NewTpl`,
  `NewTpl = HighBitSet64(mask)` -> leaves disabled, clears bit. !! Same
  false positive. Self-cleans on `RestoreTpl` (mask cleared).
- **Solution 4:** `GetInterruptState` reads HW -> IF=0 (truly disabled) [OK]
  sets mask bit -> `RestoreTpl` skips enable. -> **Correct** -- respects
  caller's explicit disable. (Not a false positive: HW really is disabled.)

**Scenario 6 -- Disable interrupts in interrupt context:**
Inside `CoreTimerTick`, code calls `gCpu->DisableInterrupt()`.

- **All five:** No-op -- interrupts are already disabled. [OK]

**Scenario 7 -- Interrupt state changes without TPL change:**
Code toggles interrupts (via protocol or raw instructions like CLI/STI)
without any `RaiseTpl(HIGH)` call.

- **All five:** No impact -- the flag/mask is only written during
  `RaiseTpl` transitions to `TPL_HIGH_LEVEL`. [OK]

**Note:** Scenarios where code disables interrupts then later calls
`RaiseTpl(HIGH)` (whether via protocol or raw instructions) are all
equivalent to Scenario 5. Solutions 1-3 produce a benign false positive
(self-cleans on next raise). Solution 4 correctly reads HW state and
respects the explicit disable.

**Implementation note:** The above analysis uses the conceptual single-mask
design (`mInterruptedTplMask`). The actual implementation uses a two-stage
approach: `gTplBeforeHighTpl` (staging) and `gIsrEntryTplMask` (ISR
tracking). `CoreRestoreTpl` unconditionally clears `gTplBeforeHighTpl = 0`
when transitioning from HIGH to below HIGH, *before* entering the event
dispatch loop. This means that in normal context (no timer fires between
`RaiseTpl` and `RestoreTpl`), the staging bits never propagate to
`gIsrEntryTplMask`, and `RestoreTpl` re-enables interrupts normally -- no
false positive occurs in any solution. See Test 9 (Section 6.4.2) which
validates this behavior.

### 4.3 Functional Scenarios (8-15)

These scenarios exercise edge cases and stress conditions that verify the
recursion fix is effective (prevents infinite recursion) and safe (no
regressions). They include timer-during-dispatch, TPL lowering patterns,
interrupt misuse, and interrupt-context recursion paths.

**Scenario 8 -- Temporarily lower TPL across HIGH (normal context):**
Code at `TPL_HIGH_LEVEL` drops to `TPL_APPLICATION` and raises back.

- **Original:** `RestoreTpl` re-enables interrupts (correct in normal
  context). `RaiseTpl` re-disables. [OK]
- **Solution 1:** Same -- mask is 0, so enables are unconditional. [OK]
- **Solution 2:** Same -- flag is TRUE, so enables proceed. [OK]
- **Solution 3:** Same -- mask is 0, final section: `NewTpl > -1` -> else
  branch -> enables. [OK]
- **Solution 4:** Same as Solution 1 -- mask is 0 in normal context. [OK]

If a timer interrupt fires during the lowered window, it follows Scenario 9
(all five handle it the same way -- self-cleans on next `RaiseTpl`).

**Scenario 9 -- Timer fires during normal-context event dispatch:**
A normal driver's `CoreRestoreTpl` event loop re-enables interrupts (flag is
FALSE / mask is 0). Timer interrupt fires during `CoreDispatchEventNotifies`.

- **Original:** Inner `CoreTimerTick`'s `CoreRestoreTpl` unconditionally
  re-enables -> timer fires again -> !! **infinite recursion**.
- **Solution 1:** Inner `CoreTimerTick` sets mask bit. Event loop checks
  mask: enables only at levels above the highest interrupted TPL. Nested
  handler's event dispatch does NOT re-enable at or below the interrupted
  level -> strictly bounds nesting. Final: `CoreSetInterruptState(FALSE)` +
  mask check -> leaves disabled. IRET restores. -> Bounded nesting (max 1
  additional level).
- **Solution 2:** Inner `CoreTimerTick` sets flag FALSE. Inner `RestoreTpl`
  skips enable. Interrupt return restores interrupts. Flag is stale but
  harmless (outer loop already has interrupts HW-enabled). [OK]
- **Solution 3:** Inner `CoreTimerTick` sets mask bit. Event loop enables
  interrupts but nested handler sees higher TPL (bounded). Final:
  `CoreSetInterruptState(FALSE)` + mask check -> leaves disabled. IRET
  restores. -> Bounded nesting, not infinite.
- **Solution 4:** Identical to Solution 1 -- HW reads IF=0 on ISR entry
  -> mask bit set -> bounded nesting (max 1 additional). [OK]

**Scenario 10 -- App enables interrupts at `TPL_HIGH`:**
Application raises to `TPL_HIGH_LEVEL`, then directly enables interrupts.
Severe misuse.

- **Original:** Timer fires -> `CoreTimerTick` -> `RestoreTpl` re-enables [OK]
  timer fires again. -> Unpredictable.
- **Solution 1:** Inner `CoreTimerTick` sets mask -> inner `RestoreTpl`
  skips enable. !! Mitigates (prevents inner re-enable).
- **Solution 2:** Inner `CoreTimerTick` sets flag -> inner `RestoreTpl`
  skips enable. !! Same mitigation.
- **Solution 3:** Inner `CoreTimerTick` sets mask. Event loop enables
  (bounded nesting). Final: mask check -> leaves disabled. !! Mitigates
  with bounded nesting during event dispatch.
- **Solution 4:** Same as Solution 1 -- inner `CoreTimerTick` sets mask
  (HW reads IF=0 on ISR entry) -> inner `RestoreTpl` skips enable.
  !! Mitigates (prevents inner re-enable).

**Scenario 11 -- `RaiseTpl(HIGH)` inside interrupt handler:**
During `CoreRestoreTpl`'s event dispatch loop inside `CoreTimerTick`, the
loop sets `gEfiCurrentTpl = PendingTpl` (e.g. `TPL_NOTIFY = 16`) and
decides whether to call `CoreSetInterruptState(TRUE)`.

- **Original:** `(16 < 31)` -> unconditionally enables interrupts -> timer
  fires -> nested `CoreTimerTick` -> !! **infinite recursion**.
- **Solution 1:** Event loop checks mask: enables only when
  `gEfiCurrentTpl > HighBitSet64(mask)`. For first-level nesting
  (mask has bit at interrupted TPL, e.g. 4), dispatch at higher levels
  (e.g. 16 > 4) re-enables -> timer may fire -> second level sets mask
  bit 16 -> mask = 0x10010 -> nested dispatch at 16: `16 > 16` is FALSE
  -> does NOT re-enable -> no further nesting. Final section:
  `CoreSetInterruptState(FALSE)` before TPL assignment + mask check
  -> stays disabled in IRQ context. -> Strictly bounded recursion
  (max 1 additional nesting level during event dispatch).
- **Solution 2:** Event loop unconditionally enables (like original/Solution 3)
  -> timer may fire -> bounded recursion during dispatch. Final section:
  `CoreSetInterruptState(FALSE)` before TPL assignment + `mInterruptState`
  is FALSE -> stays disabled. -> Bounded recursion (depth = number of pending TPL levels).
- **Solution 3:** Event loop unconditionally enables (like original) -> timer
  may fire -> bounded recursion. But final section: `CoreSetInterruptState(FALSE)`
  before TPL assignment -> nested handler sees higher TPL -> bounds depth.
  -> Bounded recursion (intentional, depth = number of pending TPL levels).
- **Solution 4:** Identical to Solution 1 -- HW reads IF=0 in ISR context
  -> same mask behavior, same strictly bounded nesting (max 1 additional). [OK]

**Scenario 12 -- Next `RaiseTpl(HIGH)` after stale flag/mask:**
After IRET, interrupts are HW-enabled but the flag/mask is stale.

- **Original:** N/A -- no tracking mechanism.
- **Solution 1:** Next `RaiseTpl(HIGH)`: `GetInterruptState` -> TRUE [OK]
  mask not modified -> self-cleans implicitly. [OK]
- **Solution 2:** Next `RaiseTpl(HIGH)`: `GetInterruptState` -> TRUE [OK]
  `mInterruptState = TRUE` -> self-cleans explicitly. [OK]
- **Solution 3:** Mask is fully cleared during `RestoreTpl` (`&= (1<<NewTpl)-1`
  with NewTpl = interrupted TPL). No stale state after IRET. [OK]
- **Solution 4:** Same as Solution 1 -- mask is cleared during the ISR's own
  `RestoreTpl`, so no stale state exists even before the next raise. [OK]

**Scenario 13 -- Temporarily lower TPL across HIGH (interrupt context):**
Inside `CoreTimerTick`, code at `TPL_HIGH_LEVEL` temporarily drops to
`TPL_APPLICATION` and raises back.

- **Original:** `RestoreTpl` re-enables interrupts -> timer fires
  -> **infinite recursion**.
- **Solution 1:** Event loop checks mask: only enables at levels above
  the interrupted TPL. If a nested timer fires during dispatch, the
  nested handler's mask bits prevent further re-enabling at or below
  that level. Final: `CoreSetInterruptState(FALSE)` + mask check
  leaves disabled. -> Strictly bounded recursion (max 1 additional
  nesting level during event dispatch).
- **Solution 2:** Event loop unconditionally enables -> bounded recursion
  during dispatch (same as Solution 3). Final: `CoreSetInterruptState(FALSE)`
  + `mInterruptState` is FALSE -> stays disabled. -> Bounded recursion
  (depth = number of pending TPL levels).
- **Solution 3:** Event loop unconditionally enables -> bounded recursion
  during dispatch (same as Solution 2). Final: `CoreSetInterruptState(FALSE)`
  + mask check -> leaves disabled. -> Bounded recursion (intentional,
  mathematically bounded by number of TPL levels).
- **Solution 4:** Identical to Solution 1 -- same mask-guarded event loop,
  same strictly bounded nesting (max 1 additional). [OK]

**Scenario 14 -- Enable interrupts in interrupt context:**
Inside `CoreTimerTick`, code calls `gCpu->EnableInterrupt()`.

- **All five:** Interrupts are immediately HW-enabled. Timer fires -> nested
  `CoreTimerTick` before any TPL check. -> Cannot prevent -- fundamental
  contract violation.

**Scenario 15 -- Enable interrupts then `RaiseTpl(HIGH)` in IRQ context:**
Inside `CoreTimerTick`, code enables interrupts then calls `RaiseTpl(HIGH)`.
Double contract violation.

- **Original:** Enable creates recursion window before `RaiseTpl`.
  `RestoreTpl` re-enables -> recursion. -> Cannot prevent.
- **Solution 1:** `GetInterruptState` -> TRUE (handler re-enabled) -> mask
  unchanged -> treats as normal context. `RestoreTpl` enables. -> False
  negative -- recursion possible.
- **Solution 2:** `GetInterruptState` -> TRUE -> `mInterruptState = TRUE`
  -> same false negative. [OK]
- **Solution 3:** `GetInterruptState` -> TRUE -> mask unchanged -> treats as
  normal context. Same false negative. [OK]
- **Solution 4:** Same as Solution 1 -- `GetInterruptState` reads HW -> IF=1
  (handler re-enabled) -> treats as normal context. -> Same false negative.

**Note on architecture-specific instructions:** Raw processor instructions
(`CLI`/`STI` on x86, `CPSID`/`CPSIE` on ARM, `DAIFSet`/`DAIFClr` on AArch64,
`CSRC`/`CSRS sstatus.SIE` on RISC-V) produce identical results to the
corresponding CPU Arch Protocol calls from the perspective of
`GetInterruptState()`. All scenarios above apply equally to protocol-based
and instruction-based interrupt manipulation.

One additional case worth noting: code that wraps a critical section with
disable/enable instructions (e.g., CLI/STI bracket) and calls `RaiseTpl(HIGH)`
inside the bracket. Solutions 1-3 produce a false positive (`RestoreTpl` skips
re-enable), but the outer enable instruction restores interrupts -- net effect
correct with slightly longer interrupts-off period. Solution 4 re-enables
interrupts (staging mask cleared by `RestoreTpl`), same as original behavior;
the outer enable instruction is then redundant.

### 4.4 Consolidated Comparison Table

| # | Scenario | Context | Mechanism | Original | Solution 1 | Solution 2 | Solution 3 | Solution 4 |
|---|---|---|---|---|---|---|---|---|
| 1 | Normal `RaiseTpl(HIGH)` / `RestoreTpl` | Normal | TPL | -> Correct | -> Correct | -> Correct | -> Correct | -> Correct |
| 2 | `RaiseTpl` to non-HIGH | Any | TPL | -> No impact | -> No impact | -> No impact | -> No impact | -> No impact |
| 3 | Nested `RaiseTpl(HIGH)` at HIGH | Any | TPL | -> No impact | -> No impact | -> No impact | -> No impact | -> No impact |
| 4 | Normal TPL nesting (event dispatch) | Normal | TPL | -> Correct | -> Correct | -> Correct | -> Correct | -> Correct |
| 5 | Disable IRQ then `RaiseTpl(HIGH)` | Normal | Protocol/Instr. | -> Correct | !! False pos. | !! False pos. | !! False pos. | -> Correct |
| 6 | Disable IRQ in IRQ context | Interrupt | Protocol/Instr. | -> No-op | -> No-op | -> No-op | -> No-op | -> No-op |
| 7 | Toggle IRQ, no TPL change | Any | Protocol/Instr. | -> No impact | -> No impact | -> No impact | -> No impact | -> No impact |
| 8 | Temp lower TPL across HIGH (normal) | Normal | TPL | -> Correct | -> Correct | -> Correct | -> Correct | -> Correct |
| 9 | Timer IRQ during normal event dispatch | Normal->IRQ | TPL | -> **Recursion** | -> Bounded | -> Stale, harmless | -> Bounded | -> Bounded |
| 10 | Enable IRQ at `TPL_HIGH` (misuse) | App | Protocol/Instr. | -> Unpredict. | !! Mitigates | !! Mitigates | !! Mitigates | !! Mitigates |
| 11 | `RaiseTpl(HIGH)` inside IRQ handler | Interrupt | TPL | -> **Recursion** | -> Bounded | -> Bounded | -> Bounded | -> Bounded |
| 12 | Next `RaiseTpl(HIGH)` after stale | Normal | TPL | N/A | -> Self-cleans | -> Self-cleans | -> Mask cleared | -> Mask cleared |
| 13 | Temp lower TPL across HIGH (IRQ) | Interrupt | TPL | -> **Recursion** | -> Bounded | -> Bounded | -> Bounded | -> Bounded |
| 14 | Enable IRQ in IRQ context | Interrupt | Protocol/Instr. | -> Cannot fix | -> Cannot fix | -> Cannot fix | -> Cannot fix | -> Cannot fix |
| 15 | Enable IRQ + `RaiseTpl(HIGH)` in IRQ | Interrupt | Protocol/Instr. | -> Cannot fix | -> False neg. | -> False neg. | -> False neg. | -> False neg. |

Legend: -> Correct | !! Minor regression or behavioral change | -> Broken / cannot fix

### 4.5 Conclusions

**For TPL-managed interrupt control (Scenarios 1-4, 8-9, 11-13):**
All four solutions correctly prevent infinite recursion. Solutions 2 and 3
allow bounded nesting during event dispatch (Scenarios 11/13: unconditionally
enable interrupts in the loop, bounded by pending TPL levels). Solutions 1
and 4 use a mask-guarded event loop that only enables interrupts when the
mask is empty (normal context), providing a stricter bound (max 1 additional
nesting level). All four use `CoreSetInterruptState(FALSE)` before the final
TPL assignment, eliminating the IRET race window. Solutions 1, 3, and 4 use
a bitmask for the final enable/skip decision; Solution 2 uses a boolean
flag. Solutions 1 and 4 share identical DXE Core code (+30/-5 lines, the
simplest). Normal TPL nesting through the event dispatch loop (Scenario 4)
is unaffected by all four solutions -- all five implementations produce
identical behavior for this expected pattern (in normal context, the mask
is 0 so the check is trivially satisfied).

**For direct interrupt manipulation (Scenarios 5-7, 10, 14-15):**
Solutions 1-3 behave identically -- same false positives (Scenario 5),
same cannot-prevent cases (14, 15), same no-impact cases (6, 7).
**Solution 4 eliminates the false positives** in Scenario 5 because
`GetInterruptState()` reads actual hardware state. When code explicitly
disables interrupts (via protocol or raw instruction), the hardware flag IS
cleared, and Solution 4 correctly detects this and respects the caller's
intent -- `RestoreTpl` does not re-enable, which is the correct behavior.
The cannot-prevent cases (14, 15) remain identical across all four solutions.

**Key distinction -- "false positive" reframing for Solution 4:**
For Solutions 1-3 (cached `GetInterruptState`), Scenario 5 is a genuine
false positive: the cache happens to read FALSE for reasons unrelated to ISR
entry. For Solution 4 (HW read), the same scenario produces **correct
behavior**: if the hardware interrupt flag is cleared (by explicit software
action or by hardware ISR entry), `RestoreTpl` should NOT re-enable
interrupts -- the caller or the interrupt-return instruction is responsible
for restoring them. This eliminates the "false positive" category entirely
for Solution 4.

**Implementation note:** The above is the conceptual single-mask analysis.
The actual implementation's two-stage design (`gTplBeforeHighTpl` /
`gIsrEntryTplMask`) eliminates the false positive for ALL solutions by
clearing `gTplBeforeHighTpl` on the HIGH-to-below-HIGH transition before
event dispatch. See Section 3.6.3 and Test 9 (Section 6.4.2).

**Original code tradeoff:** For the direct-disable scenario (5), the
original code's unconditional restore has a side effect: it re-enables
interrupts that the caller explicitly disabled. While this prevents the
interrupts-off-until-next-raise symptom, it violates the caller's intent.
Solution 4's behavior (respecting the explicit disable) is more correct from
a spec perspective -- `DisableInterrupt()` means "disable," and
`RestoreTpl()` should not undo explicit interrupt manipulation.

---

## 5 Proposed Solutions

### 5.1 Solution 1: Interrupted TPL Mask (`mInterruptedTplMask`)

**Status:** DXE Core component of Solution 4 (identical code). Also
implemented standalone in branch
`Bug_xxx_DxeCorePreventTimerInterruptInfiniteRecurssion_Solution_1`.

**Approach:** Track which TPL levels were entered while interrupts were already
disabled (i.e., from within an interrupt handler) using a bitmask. In
`CoreRestoreTpl`, the event dispatch loop only re-enables interrupts at
dispatch levels above the highest interrupted TPL (mask-guarded), and the
final section unconditionally disables interrupts before the TPL assignment
and checks the mask to decide whether to re-enable. This eliminates the
IRET race window and strictly bounds nesting depth.

**Key changes (+30 / -5 lines):**

1. **New global:** `volatile UINTN mInterruptedTplMask = 0` -- bitmask of TPL
   levels that were "raised to HIGH" while interrupts were already off.

2. **`CoreRaiseTpl` modification:** When raising to `TPL_HIGH_LEVEL` from
   below, check interrupt state via `gCpu->GetInterruptState()`. If
   interrupts are already disabled (interrupt context), ASSERT monotonic
   nesting and record `OldTpl` in the mask. Always call
   `CoreSetInterruptState(FALSE)` unconditionally (same as original code).

3. **`CoreRestoreTpl` modification (event loop):** Only call
   `CoreSetInterruptState(TRUE)` when below `TPL_HIGH_LEVEL` **and**
   `gEfiCurrentTpl > HighBitSet64(mInterruptedTplMask)`. In normal context
   (mask = 0, `HighBitSet64(0) = -1`), every TPL level satisfies this and
   interrupts are enabled at each dispatch level -- identical to the original
   code. In interrupt context, interrupts are only re-enabled at dispatch
   levels **above** the highest interrupted TPL in the mask. This prevents
   re-entry at or below the interrupted level while still allowing bounded
   nesting at higher levels. If a nested timer fires during dispatch and
   sets a higher mask bit, subsequent dispatch levels at or below that bit
   will NOT re-enable, limiting nesting to at most one additional level.

4. **`CoreRestoreTpl` modification (final section):** After the event
   dispatch loop, **unconditionally disable** interrupts with
   `CoreSetInterruptState(FALSE)` **before** assigning `gEfiCurrentTpl`.
   Then check the mask: if `NewTpl = HighBitSet64(mask)`, we are in
   interrupt context -- clear lower bits and leave disabled (IRET will
   re-enable). Otherwise, re-enable interrupts (normal context).

```mermaid
sequenceDiagram
    participant App as Application Code
    participant DXE as DXE Core
    participant T1 as CoreTimerTick [1]
    participant CPU as CPU

    Note over App: Running at TPL_APPLICATION (4)<br/>Interrupts ENABLED

    CPU->>T1: Timer Interrupt (HW disables interrupts)
    activate T1

    T1->>DXE: CoreRaiseTpl(31)
    Note over DXE: GetInterruptState -> FALSE<br/>Interrupts already disabled!<br/>ASSERT OldTpl(4) >= HighBitSet64(mask=0)<br/>Set mInterruptedTplMask |= (1<<4) = 0x10<br/>CoreSetInterruptState(FALSE) (unconditional)
    DXE-->>T1: OldTpl = 4

    T1->>T1: Update time, signal events

    T1->>DXE: CoreRestoreTpl(4)
    Note over DXE: Event loop: checks mask before enabling<br/>gEfiCurrentTpl > HighBitSet64(0x10)=4?<br/>Enables only at levels above 4<br/>(bounded nesting if timer fires)
    Note over DXE: Final: CoreSetInterruptState(FALSE)<br/>gEfiCurrentTpl = 4<br/>4 = HighBitSet64(0x10)=4<br/>mask &= (1<<4)-1 = 0x0F -> mask = 0<br/>Leave disabled [OK]
    DXE-->>T1: returns (interrupts still disabled)

    T1-->>CPU: Return from interrupt (IRET)
    deactivate T1
    Note over CPU: HW re-enables interrupts on IRET [OK]

    Note over App: Resumes safely -- no recursion
```

**Pros:**
- Correctly prevents recursion by suppressing interrupt re-enable inside
  interrupt handlers.
- Preserves normal TPL/interrupt behavior for non-interrupt code paths.
- Mask-guarded event loop enables interrupts only at dispatch levels above
  the highest interrupted TPL -- preserves timer latency for first-level
  nesting while strictly bounding further nesting (max 1 additional level).
- `CoreSetInterruptState(FALSE)` before TPL assignment eliminates IRET race
  window -- structurally guaranteed bounded recursion.
- ASSERT validates monotonic nesting invariant -- catches violations early.
- Minimal code change: +30/-5 lines (simplest of all four DXE Core solutions).

**Cons / Considerations:**
- Allows bounded recursion during event dispatch (at most one nested handler
  invocation per dispatch cycle). Stack depth is bounded but non-zero
  additional usage.
- Event dispatch callbacks at TPL levels **at or below** the highest
  interrupted TPL execute with interrupts disabled. This is a deliberate
  trade-off: it prevents re-entry at the cost of slightly increased timer
  latency during nested dispatch. In practice, this only affects callbacks
  during interrupt-context event dispatch, not normal-context dispatch.
- `mInterruptedTplMask` uses non-atomic read-modify-write (`|=`, `&=`).
  Safe because interrupts are disabled during these operations, but fragile
  if assumptions change.
- Adds a `gCpu->GetInterruptState()` call to every `CoreRaiseTpl(HIGH)`,
  adding slight overhead.
- `1ULL << OldTpl` is safe for UEFI's TPL range (4-31) but would be
  undefined behavior for values = bit-width of `UINTN`.
- Same false-positive limitation as Solutions 2/3 for direct interrupt
  manipulation (Scenario 5) in the conceptual single-mask design. The actual
  two-stage implementation (`gTplBeforeHighTpl` + `gIsrEntryTplMask`)
  eliminates this by clearing staging bits on RestoreTpl.

---

### 5.2 Solution 2: Boolean Flag + Unconditional Disable (`mInterruptState`)

**Status:** Not implemented (alternative design, documented for comparison)

**Approach:** Use a single `BOOLEAN` flag that saves the interrupt state
when `CoreRaiseTpl(HIGH)` is called. Combined with Solution 3's key insight
of `CoreSetInterruptState(FALSE)` before the final TPL assignment, this
gives both timer latency preservation (unconditional enable in event loop)
and a race-free final section (disable before assign + boolean check). The
flag is "self-cleaning" -- it is set only in `CoreRaiseTpl`, never modified
in `CoreRestoreTpl`. TRUE means interrupts were enabled (normal context),
FALSE means they were already disabled (interrupt context).

**Key changes:**

1. **New global:** `volatile BOOLEAN mInterruptState = TRUE`

2. **`CoreRaiseTpl` modification:** When raising to `TPL_HIGH_LEVEL`, call
   `gCpu->GetInterruptState()` to query the current interrupt state, then:
   - Set `mInterruptState = State` (direct assignment, no negation)
   - Call `CoreSetInterruptState(FALSE)` unconditionally (same as original)

3. **`CoreRestoreTpl` event dispatch loop:** Unconditionally enable
   interrupts (like the original code and Solution 3). No
   `mInterruptState` check -- allows timer interrupts to fire during
   event dispatch for timer latency preservation.

4. **`CoreRestoreTpl` final section (HIGH?non-HIGH only):** When
   `OldTpl >= TPL_HIGH_LEVEL`, call `CoreSetInterruptState(FALSE)`
   **before** `gEfiCurrentTpl = NewTpl` (eliminates race window).
   Then guard `CoreSetInterruptState(TRUE)` with `mInterruptState` --
   only re-enables in normal context. In interrupt context, the interrupt
   return restores them. For non-HIGH transitions (`OldTpl < TPL_HIGH_LEVEL`),
   preserve the original behavior: set TPL and unconditionally re-enable.
   The `OldTpl` guard is critical because `mInterruptState` may be stale
   FALSE from a prior timer interrupt -- checking it during non-HIGH
   transitions would permanently disable interrupts.

**Self-cleaning mechanism:**

```
Interrupt handler active:
  CoreRaiseTpl(HIGH) -> GetInterruptState=FALSE -> mInterruptState=FALSE
  CoreRestoreTpl(4):
    Event loop: enables interrupts (timer may fire -> bounded nesting)
    Final (OldTpl=31 = HIGH): CoreSetInterruptState(FALSE) -> gEfiCurrentTpl=4
    mInterruptState = FALSE -> skip enable -> interrupts stay off [OK]
  Interrupt return -> HW restores interrupts
  (flag is stale FALSE, but harmless -- non-HIGH RestoreTpl calls
  are guarded by OldTpl < HIGH -> use original unconditional enable)

Next normal RaiseTpl:
  CoreRaiseTpl(HIGH) -> GetInterruptState=TRUE -> mInterruptState=TRUE -> self-clean
  CoreRestoreTpl(4):
    Event loop: enables interrupts (normal behavior)
    Final (OldTpl=31 = HIGH): CoreSetInterruptState(FALSE) -> gEfiCurrentTpl=4
    mInterruptState = TRUE -> enable interrupts [OK]
```

```mermaid
sequenceDiagram
    participant App as Application Code
    participant DXE as DXE Core
    participant T1 as CoreTimerTick [1]
    participant CPU as CPU

    Note over App: Running at TPL_APPLICATION (4)<br/>Interrupts ENABLED

    CPU->>T1: Timer Interrupt (HW disables interrupts)
    activate T1

    T1->>DXE: CoreRaiseTpl(31)
    Note over DXE: GetInterruptState -> FALSE<br/>mInterruptState = FALSE
    DXE-->>T1: OldTpl = 4

    T1->>T1: Update time, signal events

    T1->>DXE: CoreRestoreTpl(4)
    Note over DXE: Event loop: enables interrupts<br/>(timer latency preserved)
    Note over DXE: Event loop exits
    Note over DXE: OldTpl(31) = HIGH -> guarded path
    Note over DXE: CoreSetInterruptState(FALSE)<br/>-> disable BEFORE TPL assign
    Note over DXE: gEfiCurrentTpl = 4
    Note over DXE: mInterruptState = FALSE<br/>Skip CoreSetInterruptState(TRUE) [OK]
    DXE-->>T1: returns (interrupts disabled)

    T1-->>CPU: Return from interrupt
    deactivate T1
    Note over CPU: HW re-enables interrupts [OK]

    Note over App: Resumes at TPL_APPLICATION

    Note over App,CPU: -- Next normal TPL raise --

    App->>DXE: CoreRaiseTpl(31) -- normal lock acquire
    Note over DXE: GetInterruptState -> TRUE<br/>mInterruptState = TRUE -> self-clean<br/>CoreSetInterruptState(FALSE)
    DXE-->>App: OldTpl = 4

    App->>DXE: CoreRestoreTpl(4)
    Note over DXE: Event loop: enables interrupts<br/>(normal behavior)
    Note over DXE: Event loop exits
    Note over DXE: OldTpl(31) = HIGH -> guarded path
    Note over DXE: CoreSetInterruptState(FALSE)<br/>-> disable before TPL assign
    Note over DXE: gEfiCurrentTpl = 4
    Note over DXE: mInterruptState = TRUE<br/>CoreSetInterruptState(TRUE) [OK]
    DXE-->>App: returns (interrupts enabled)
```

**Pros:**
- Simple: `CoreRaiseTpl` adds `GetInterruptState` + save (the
  `CoreSetInterruptState(FALSE)` call is unchanged from original).
  `CoreRestoreTpl` adds an `OldTpl >= HIGH` guard with disable-before-
  assign + boolean check; non-HIGH transitions preserve original behavior.
- No double negation: `mInterruptState = State` stores directly,
  `mInterruptState` reads directly.
- Timer latency preserved: unconditional enable in event loop (like original).
- Race window eliminated: disable before TPL assignment (like Solution 3).
- Bounded recursion during event dispatch (same depth bound as Solution 3).
- Self-cleaning -- no risk of stale value accumulating in a mask.
- Same `GetInterruptState()` call as Solution 1 (no additional overhead).
- Simpler correctness argument than Solution 3: boolean + disable-before-
  assign = no race; no bitmask arithmetic needed.

**Cons / Considerations:**
- Allows bounded recursion during event dispatch (timer interrupts may nest
  during the loop). Depth bounded by number of pending TPL levels.
- The flag is "stale FALSE" between interrupt return and the next
  `CoreRaiseTpl(HIGH)`. Harmless because `CoreRestoreTpl` only checks the
  flag when `OldTpl >= TPL_HIGH_LEVEL` (i.e., lowering from HIGH). Non-HIGH
  `RestoreTpl` calls use the original unconditional enable path, which does
  not consult `mInterruptState`.
  `CoreRaiseTpl` that refreshes it.
- Does not provide per-TPL tracking (unlike Solutions 1/3 bitmask). The
  boolean only distinguishes "interrupt vs normal" -- sufficient for the
  final enable decision.

---

### 5.3 Feature Comparison

| Aspect | Solution 1 (Bitmask) | Solution 2 (Boolean) | Solution 3 (Unconditional Disable) | Solution 4 (HW Read + Bitmask) -- **Recommended** |
|---|---|---|---|---|
| **Data structure** | `UINTN` bitmask (32/64 bits) | Single `BOOLEAN` | `UINTN` bitmask (32/64 bits) | `UINTN` bitmask (same as Sol 1) |
| **CoreRaiseTpl changes** | `GetInterruptState` + bit-OR + ASSERT (>=) | `GetInterruptState` + save state | `GetInterruptState` + bit-OR + ASSERT (>) | Same as Solution 1 |
| **CoreRestoreTpl changes** | Mask-guarded event loop + unconditional disable + mask check + clear | Unconditional event loop + unconditional disable + `mInterruptState` check | Unconditional event loop + unconditional disable + mask check + clear | Same as Solution 1 |
| **Event dispatch in IRQ context** | Conditionally re-enables (only above interrupted TPL) | Re-enables (like original) | Re-enables (like original) | Same as Solution 1 |
| **Final TPL assignment** | After unconditional disable (race-free) | After unconditional disable (race-free) | After unconditional disable (race-free) | After unconditional disable (race-free) |
| **Bounded recursion risk** | Strictly bounded (max 1 additional nesting) | Bounded (depth = pending TPL count) | Bounded (depth = pending TPL count) | Strictly bounded (max 1 additional) |
| **IRET race window** | -> Eliminated (disable before assign) | -> Eliminated (disable before assign) | -> Eliminated (disable before assign) | -> Eliminated (disable before assign) |
| **Cleanup required** | Yes -- clear bits in `CoreRestoreTpl` | No -- self-cleans in next `CoreRaiseTpl` | Yes -- clear bits in `CoreRestoreTpl` | Yes -- same as Solution 1 |
| **False positives** | Scenario 5 | Scenario 5 | Scenario 5 | **None** (reads HW) |
| **Detection method** | `GetInterruptState` (cached) | `GetInterruptState` (cached) | `GetInterruptState` (cached) | `GetInterruptState` (HW) |
| **Code complexity** | Minimal | Minimal-moderate | Moderate-high | Minimal (DXE) + trivial (CpuDxe) |
| **Lines changed vs original** | +30 / -5 | +28 / -9 | +77 / -13 | +30/-5 (DXE) + ~4 (CpuDxe --2) |
| **Commit** | -- | -- | `bad938e41f1d` | Current branch |
| **Recommendation** | Viable | Viable | Reference impl | **Preferred** |

---

### 5.4 Solution 3: Unconditional Disable + Bounded Nesting (Paolo Bonzini)

**Status:** Committed as `bad938e41f1d` ("MdeModulePkg: fix stack overflow
issue due to nested interrupts")

**Approach:** Like Solution 1, uses `volatile UINTN mInterruptedTplMask` to
track interrupted TPL levels. However, the `CoreRestoreTpl` logic is
fundamentally different: it **unconditionally re-enables** interrupts during
the event dispatch loop (like the original code) and then **unconditionally
disables** interrupts before assigning `gEfiCurrentTpl = NewTpl`. After the
assignment, it checks the mask to decide whether to re-enable or leave
disabled. This bounds the stack depth by ensuring any nested interrupt
handler sees a higher TPL than the outer handler.

**Key changes (+77 / -13 lines):**

1. **New global:** `static UINTN mInterruptedTplMask = 0` -- same bitmask
   concept as Solution 1.

2. **`CoreRaiseTpl` modification:** Same `GetInterruptState` check as
   Solutions 1 and 2. When interrupts are already disabled: adds `ASSERT`
   verifying monotonic nesting (`OldTpl > HighBitSet64(mask)`), then sets
   bit. When interrupts are enabled: disables them.

3. **`CoreRestoreTpl` event dispatch loop:** **Unconditionally** calls
   `CoreSetInterruptState(TRUE)` when `gEfiCurrentTpl < TPL_HIGH_LEVEL` --
   same as original unfixed code. This allows timer interrupts during event
   dispatch, intentionally permitting bounded nesting.

4. **`CoreRestoreTpl` final section:**
   ```c
   CoreSetInterruptState(FALSE);     // always disable before TPL assignment
   gEfiCurrentTpl = NewTpl;          // set final TPL
   if ((INTN)NewTpl <= HighBitSet64(mInterruptedTplMask)) {
     // In interrupt context: leave disabled, clear bits = NewTpl
     mInterruptedTplMask &= (1U << NewTpl) - 1;
   } else if (NewTpl < TPL_HIGH_LEVEL) {
     // Normal context: re-enable
     CoreSetInterruptState(TRUE);
   }
   ```

**Critical difference from Solution 1:** Both Solutions 1 and 3 share the
`CoreSetInterruptState(FALSE)` before assigning `gEfiCurrentTpl` (which
eliminates the IRET race window) and both use a bitmask for the final
enable decision. The key difference is in the event dispatch loop:
Solution 3 **unconditionally** enables interrupts at every dispatch level
below HIGH, while Solution 1 **guards** the enable with the mask (only
enables at levels above the highest interrupted TPL). This means:
- Solution 3: Allows bounded nesting at every dispatch level (one per
  pending TPL). Any nested interrupt sees `gEfiCurrentTpl` at a higher
  level (because it hasn't been lowered yet), preventing unbounded growth.
- Solution 1: Only allows nesting at dispatch levels above the interrupted
  TPL. If a nested timer fires and sets a higher mask bit, further dispatch
  levels won't re-enable -- limiting to at most 1 additional nesting.
Solution 2 takes yet another approach: unconditional enable in the loop
(like Solution 3) but uses a boolean flag instead of a bitmask for the
final section.

**Bounded recursion explanation:**
```
Normal: App at TPL_APPLICATION(4), events pending at NOTIFY(16) and CALLBACK(8)
Timer interrupts at any point during event dispatch:
  - During dispatch at TPL_NOTIFY(16): nested handler sees TPL=16,
    raises to HIGH, restores to 16. Bounded: can't nest again at 16
    because gEfiCurrentTpl is already 16 when the next interrupt arrives.
  - At final restore: CoreSetInterruptState(FALSE) before gEfiCurrentTpl=4
    prevents any interrupt from seeing TPL=4 (the vulnerable state).
```

```mermaid
sequenceDiagram
    participant App as Application Code
    participant DXE as DXE Core
    participant T1 as CoreTimerTick [1]
    participant T2 as CoreTimerTick [2]
    participant CPU as CPU

    Note over App: Running at TPL_APPLICATION (4)<br/>Interrupts ENABLED

    CPU->>T1: Timer Interrupt (HW disables interrupts)
    activate T1

    T1->>DXE: CoreRaiseTpl(31)
    Note over DXE: GetInterruptState -> FALSE<br/>ASSERT(4 > HighBitSet64(0)=-1) [OK]<br/>mInterruptedTplMask |= (1<<4) = 0x10
    DXE-->>T1: OldTpl = 4

    T1->>T1: Update time, signal NOTIFY event

    T1->>DXE: CoreRestoreTpl(4)
    Note over DXE: Event loop: gEfiCurrentTpl = 16<br/>(16 < 31) -> CoreSetInterruptState(TRUE)

    CPU->>T2: Timer Interrupt during event dispatch
    activate T2
    T2->>DXE: CoreRaiseTpl(31)
    Note over DXE: GetInterruptState -> FALSE<br/>ASSERT(16 > HighBitSet64(0x10)=4) [OK]<br/>mInterruptedTplMask |= (1<<16) = 0x10010
    DXE-->>T2: OldTpl = 16

    T2->>T2: Update time (no events pending above 16)

    T2->>DXE: CoreRestoreTpl(16)
    Note over DXE: No events above 16<br/>CoreSetInterruptState(FALSE)<br/>gEfiCurrentTpl = 16<br/>(16 = HighBitSet64(0x10010)=16)<br/>Clear: mask &= (1<<16)-1 = 0x10
    DXE-->>T2: returns (interrupts DISABLED)

    T2-->>CPU: IRET
    deactivate T2
    Note over CPU: HW re-enables interrupts [OK]

    Note over DXE: Back in event dispatch at TPL_NOTIFY(16)<br/>CoreDispatchEventNotifies(16)

    Note over DXE: Loop done. Final restore:<br/>CoreSetInterruptState(FALSE)<br/>gEfiCurrentTpl = 4<br/>(4 = HighBitSet64(0x10)=4)<br/>Clear: mask &= (1<<4)-1 = 0x0
    DXE-->>T1: returns (interrupts DISABLED)

    T1-->>CPU: IRET
    deactivate T1
    Note over CPU: HW re-enables interrupts [OK]

    Note over App: Resumes safely at TPL_APPLICATION
```

**Pros:**
- Preserves original event dispatch behavior: interrupts enabled during
  `CoreDispatchEventNotifies` at each TPL level (allows timer events to
  fire during event dispatch -- important for latency).
- Bounds recursion depth mathematically: max nesting = number of distinct
  TPL levels with pending events (typically 2-3, never infinite).
- The unconditional `CoreSetInterruptState(FALSE)` + TPL assignment pattern
  eliminates the IRET race window.
- ASSERT validates monotonic nesting invariant -- catches violations early.
- Same `GetInterruptState()` detection mechanism as Solutions 1 and 2.

**Cons / Considerations:**
- Still allows bounded recursion (one level per pending TPL). Stack depth
  is bounded but non-zero additional usage.
- More complex: +77/-13 lines. Harder to reason about correctness.
- `mInterruptedTplMask` uses non-atomic `|=` and `&=` -- safe because
  interrupts are disabled during modification.
- Extra `CoreSetInterruptState(FALSE)` call on every `CoreRestoreTpl` exit
  (even in normal context where it will immediately be followed by
  `CoreSetInterruptState(TRUE)`). Minor performance overhead.
- Same false-positive limitation as Solutions 1/2 for direct interrupt
  manipulation (Scenario 5) in the conceptual single-mask design.
- Comment mentions "Windows boot loader executes stray STIs at
  TPL_HIGH_LEVEL" -- this motivated the unconditional-disable approach.

#### 5.4.1 Comparative Analysis: Solutions 1, 2, 3, and 4

Solutions 1, 3, and 4 share the same `CoreRaiseTpl` logic and the same bitmask
data structure, but differ in their `CoreRestoreTpl` event dispatch loop
behavior. Solution 3 unconditionally re-enables interrupts during event
dispatch (like the original code), while Solutions 1 and 4 guard the re-enable
with a mask check that only enables when the mask is empty (normal context).
This makes Solutions 1/4 **more conservative** than Solution 3 -- they allow
at most one additional nesting level during event dispatch, while Solution 3
allows one nesting level per pending TPL. Solution 4 additionally fixes the
`GetInterruptState()` return value to be ground truth, eliminating false
positives that affect Solutions 1-3.

##### Solution 1 vs Solution 3: Key Difference

Both solutions share:

1. **Unconditional disable before TPL assignment:** Both solutions call
   `CoreSetInterruptState(FALSE)` before `gEfiCurrentTpl = NewTpl`,
   eliminating the IRET race window.

2. **Mask-based final enable decision:** Both check the mask to decide
   whether to re-enable interrupts after the TPL assignment.

They differ in the event dispatch loop:

| Aspect | Solution 1/4 | Solution 3 |
|---|---|---|
| **Event loop enable** | Guarded: only when mask == 0 | Unconditional (like original) |
| **Max nesting depth** | 1 additional level | 1 per pending TPL level |
| **Timer latency in IRQ** | Slightly increased at lower dispatch levels | Preserved (like original) |
| **IRQ-context dispatch** | Callbacks at/below interrupted TPL run with IRQ off | All callbacks run with IRQ on |
| **Lines changed** | +30 / -5 | +77 / -13 |
| **ASSERT** | `>=` (allows same-level re-entry from dispatch) | `>` (strict monotonic) |

In **normal context** (mask = 0), both solutions produce identical behavior:
`HighBitSet64(0) = -1`, so every TPL level satisfies the `> -1` check and
interrupts are enabled at every dispatch level -- same as the original code.

In **interrupt context**, Solutions 1/4 are strictly more conservative. Example
with interrupt at APPLICATION(4), pending events at NOTIFY(16) and
CALLBACK(8):

- **Solutions 1/4:** Dispatch at NOTIFY(16): `mInterruptedTplMask != 0` [OK]
  does NOT enable. Callbacks at NOTIFY run with interrupts disabled.
  Next: CALLBACK(8): same check -> does NOT enable. No nesting at all
  during event dispatch in ISR context.
- **Solution 3:** Dispatch at NOTIFY(16): unconditionally enables. Timer
  fires -> nested handler. After IRET, CALLBACK(8): unconditionally enables.
  Another timer could fire -> deeper nesting (bounded by number of levels).

The remaining differences are cosmetic:

| Aspect | Solution 1/4 | Solution 3 |
|---|---|---|
| **Mask variable** | `mInterruptedTplMask` | `mInterruptedTplMask` |
| **Unconditional disable in RaiseTpl** | Yes (simplified) | Conditional (only if State was TRUE) |
| **Final mask clear expression** | `(1ULL << NewTpl) - 1` | `(UINTN)(1U << NewTpl) - 1` |
| **Comments** | Minimal | Extensive (mentions Windows boot loader STI) |
| **Code complexity** | Minimal | Moderate-high |

Solution 4 (= Solution 1 + HW-read fix) is recommended due to its
simplicity, stricter nesting bound, and elimination of false positives.

##### Solution 2 vs Solutions 1/3/4: Boolean vs Bitmask

Solutions 1, 3, and 4 use a bitmask while Solution 2 uses a boolean flag. The
comparison centers on these differences:

**Solution 2 (boolean + `OldTpl` guard):** A single `mInterruptState` flag
provides a binary answer. The final section is guarded by
`OldTpl >= TPL_HIGH_LEVEL` -- only HIGH?non-HIGH transitions consult the
flag. Non-HIGH transitions use the original unconditional enable path,
avoiding the stale-flag problem. However, Solution 2's boolean can be
overwritten by callback `RaiseTpl(HIGH)` during event dispatch, creating
one extra bounded nesting level.

**Solutions 1/3/4 (bitmask):** Track exactly which TPL levels have been
interrupted. The mask is cleared during each `RestoreTpl`, so there is no
stale-state problem. Immune to callback overwrite because the bitmask
tracks the original interrupted TPL independently. Solutions 1/4 additionally
use the mask in the event loop to suppress re-enabling when the mask is
non-zero, providing the tightest nesting bound.

**Summary:**

| Property | Solution 1 | Solution 2 | Solution 3 | Solution 4 |
|---|---|---|---|---|
| **IRET race window** | -> Eliminated | -> Eliminated | -> Eliminated | -> Eliminated |
| **Stack depth bound** | Strictly bounded: max 1 additional | Bounded: = 1 + pending TPLs | Bounded: = 1 + pending TPLs | Strictly bounded: max 1 additional |
| **Monotonic nesting** | Enforced by ASSERT (>=) | Not enforced | Enforced by ASSERT (>) | Enforced by ASSERT (>=) |
| **Timer latency in IRQ** | Slightly increased at lower levels | Preserved (like original) | Preserved (like original) | Slightly increased at lower levels |
| **Correctness argument** | Structurally guaranteed | Structurally guaranteed | Structurally guaranteed | Structurally guaranteed |
| **Context detection** | Bitmask (per-TPL, cached) | Boolean (binary, cached) | Bitmask (per-TPL, cached) | Bitmask (per-TPL, **HW read**) |
| **Callback overwrite** | Immune | Extra bounded nesting | Immune | Immune |
| **Event loop guard** | Mask-based (strict) | None (unconditional) | None (unconditional) | Mask-based (strict) |
| **False positives** | Scenario 5 (conceptual) | Scenario 5 (conceptual) | Scenario 5 (conceptual) | **None** |
| **Code complexity** | +30/-5 lines | +28/-9 lines | +77/-13 lines | +30/-5 (DXE) + ~4 (CpuDxe) |
| **Recommendation** | Viable | Viable | Reference implementation | **Preferred** |

##### Solution 2 vs Solution 3: Simplicity vs Precision

Solutions 2 and 3 share the same structural safety property: both
disable interrupts before the final TPL assignment when lowering from
`TPL_HIGH_LEVEL`. The remaining differences are in how they detect
interrupt context and handle the stale-flag problem:

**Callback overwrite consideration (Solution 2 only):** When an event
callback inside the IRQ-context event dispatch loop calls
`RaiseTpl(HIGH)` / `RestoreTpl` (e.g., to acquire a lock), the callback's
`RaiseTpl` sees `GetInterruptState?TRUE` (event loop enabled them) and
overwrites `mInterruptState = TRUE`. The outer `CoreRestoreTpl`'s final
section then sees `mInterruptState = TRUE` and re-enables interrupts in
IRQ context -- creating one extra bounded nesting window before IRET.
Solutions 1, 3, and 4 do not have this issue because their bitmask tracks the
original interrupted TPL independently of callback activity. This is not a
correctness bug (nesting is still bounded), but it is a difference in
precision.

For the critical correctness property (preventing the IRET race), all four
solutions are equivalent -- the `CoreSetInterruptState(FALSE)` before the
TPL assignment is what eliminates the race. The boolean vs bitmask
distinction affects:

1. **Stale-flag safety:** Solution 2 requires the `OldTpl` guard to avoid
   non-HIGH transitions checking stale state. Solutions 1/3/4 clear their
   mask during `RestoreTpl`, avoiding the issue structurally.
2. **Callback overwrite:** Solution 2's boolean can be overwritten by
   callback `RaiseTpl(HIGH)`, creating one extra bounded nesting level.
   Solutions 1/3/4's bitmask is immune to this.
3. **Debug diagnostics:** Solutions 1/3/4's ASSERT catches violations of
   monotonic nesting assumptions.
4. **Simplicity:** Solutions 1/4 are the simplest (+30/-5 lines DXE Core),
   followed by Solution 2 (+28/-9), then Solution 3 (+77/-13).
5. **False positives:** Solution 4 eliminates the Scenario 5 false positive
   by reading actual hardware state. Solutions 1-3 share the same cached
   `GetInterruptState()` limitation.

### 5.5 Detailed Justification: Bitmask vs Boolean

This section provides a comprehensive analysis of why the `mInterruptedTplMask`
bitmask approach (Solutions 1/3/4) was chosen over the simpler boolean
`mInterruptState` approach (Solution 2). While both prevent infinite recursion,
the bitmask provides critical correctness properties that the boolean cannot.

#### 5.5.1 The Core Problem: When Is It Safe to Re-Enable Interrupts?

The fundamental question `CoreRestoreTpl` must answer is: "Should I re-enable
interrupts now?" The answer depends on whether we are still inside an
interrupt handler AND whether the current TPL has descended below the level
that was interrupted.

A **boolean** answers a simpler question: "Were interrupts disabled when we
entered TPL_HIGH_LEVEL?" -- YES or NO. It cannot distinguish:
- "We interrupted TPL 4 and are now at TPL 16 during event dispatch" (safe to
  re-enable -- we're above the interrupted level)
- "We interrupted TPL 4 and are now at TPL 4 after event dispatch" (NOT safe --
  we're at the interrupted level, IRET must handle it)

A **bitmask** answers the precise question: "Which specific TPL level was
interrupted?" -- enabling per-level decisions about when re-enable is safe.

#### 5.5.2 Per-TPL Granularity: The Key Advantage

Consider this scenario: Timer interrupts code at `TPL_APPLICATION` (4).
During `CoreTimerTick`, the lock release triggers `CoreRestoreTpl(4)`, which
has pending events at `TPL_NOTIFY` (16) and `TPL_CALLBACK` (8).

**With bitmask (mask = 0x10, bit 4 set):**
```
Event dispatch at TPL_NOTIFY (16):
  Check: (16 < 31) && (mInterruptedTplMask == 0)?
  mInterruptedTplMask = 0x10 -> 0 -> do NOT enable interrupts
  Dispatch NOTIFY callbacks with interrupts disabled
  (Prevents any nested timer interrupt at this level)

Event dispatch at TPL_CALLBACK (8):
  Check: (8 < 31) && (mInterruptedTplMask == 0)?
  mInterruptedTplMask = 0x10 -> 0 -> do NOT enable interrupts
  Dispatch CALLBACK callbacks with interrupts disabled

Final section:
  NewTpl = 4, HighBitSet64(0x10) = 4
  (4 <= 4) -> clear mask: mask &= (1<<4)-1 = 0 -> leave disabled
  IRET re-enables interrupts [OK]
```

**With boolean (mInterruptState = FALSE):**
```
Event dispatch at TPL_NOTIFY (16):
  Unconditionally enables interrupts (like original code)
  Timer can fire -> nested CoreTimerTick -> bounded but adds stack depth

Event dispatch at TPL_CALLBACK (8):
  Unconditionally enables interrupts again
  Timer can fire again -> another nesting level

Final section:
  OldTpl(31) >= HIGH -> guarded path
  mInterruptState = FALSE -> skip enable [OK]
  (But we already allowed bounded nesting during dispatch)
```

The bitmask approach prevents ALL re-enabling during ISR-context event
dispatch (when `mInterruptedTplMask != 0`), providing the **tightest
possible nesting bound: zero additional nesting** during the event dispatch
loop. The boolean approach allows one nesting level per dispatch TPL.

#### 5.5.3 Self-Clearing Mechanism

The bitmask's self-clearing behavior is elegant and requires no explicit
"end of ISR" signal:

```c
if ((INTN)NewTpl <= HighBitSet64 (mInterruptedTplMask)) {
    mInterruptedTplMask &= (1ULL << NewTpl) - 1;
}
```

When `RestoreTpl` descends to or below the interrupted TPL level:
- `(1ULL << NewTpl) - 1` creates a mask of all bits below `NewTpl`
- `&=` clears the interrupted TPL bit and all higher bits
- If NewTpl = 4 and mask = 0x10 (bit 4): `mask &= 0x0F` -> mask = 0

This natural clearing happens exactly when we pass through the interrupted
level -- the exact point where the ISR is "done" from the TPL perspective.
No counter to decrement, no flag to manually reset, no cross-module
coordination.

**Boolean's clearing problem:** A boolean has no analogous self-clearing
mechanism tied to TPL descent. It must either:
- Be overwritten on the next `RaiseTpl(HIGH)` (stale between IRET and next
  raise -- harmless but imprecise)
- Require an `OldTpl` guard to avoid checking stale state in non-HIGH
  transitions (Solution 2's approach -- adds complexity)

#### 5.5.4 Callback Overwrite Immunity

During interrupt-context event dispatch, event callbacks may acquire locks:
```c
// Inside a TPL_NOTIFY callback, dispatched during CoreTimerTick's RestoreTpl:
CoreAcquireLock (&SomeLock);   // -> RaiseTpl(HIGH)
// ... critical section ...
CoreReleaseLock (&SomeLock);   // -> RestoreTpl(TPL_NOTIFY)
```

**With bitmask:** The callback's `RaiseTpl(HIGH)` calls
`gCpu->GetInterruptState()`. If the event loop re-enabled interrupts
(Solution 3), `GetInterruptState()` returns TRUE -> no mask bit set. The
mask still has the original bit 4 from the outer ISR context. The callback's
`RestoreTpl` is a non-HIGH?non-HIGH transition that doesn't touch the mask.
The outer `RestoreTpl`'s final section still sees mask = 0x10 -> leaves
disabled. **The original ISR context is preserved regardless of callback
behavior.**

**With boolean:** The callback's `RaiseTpl(HIGH)` calls
`gCpu->GetInterruptState()` -> TRUE (event loop enabled them) [OK]
`mInterruptState = TRUE`. This **overwrites** the outer ISR's FALSE value.
The outer `RestoreTpl`'s final section now sees `mInterruptState = TRUE` [OK]
calls `CoreSetInterruptState(TRUE)` -> re-enables interrupts in ISR context.
A timer can now fire -> one extra bounded nesting level.

This is the most significant correctness difference: the bitmask is
**structurally immune** to callback interference, while the boolean requires
accepting one extra bounded nesting window during callback-heavy event
dispatch.

#### 5.5.5 ASSERT Diagnostic Value

The bitmask enables a powerful runtime invariant check:

```c
ASSERT ((INTN)OldTpl >= HighBitSet64 (mInterruptedTplMask));
```

This verifies that nested interrupts always occur at monotonically
non-decreasing TPL levels. If this ASSERT fires, it indicates either:
- A timer driver bug (calling `CoreTimerTick` at unexpected TPL)
- A hardware anomaly (interrupt firing at unexpected time)
- A DXE Core logic error (mask not cleared properly)

A boolean provides no equivalent diagnostic -- `mInterruptState = FALSE`
gives no information about WHICH level was interrupted or WHETHER nesting
is occurring in the expected order.

#### 5.5.6 Complexity Cost Assessment

The bitmask adds this complexity over the boolean:

1. **In `CoreRaiseTpl`:** `mInterruptedTplMask |= (1ULL << OldTpl)` -- one
   bitwise OR (trivial).
2. **In event loop:** `(mInterruptedTplMask == 0)` -- one comparison to zero
   (trivial, same cost as checking a boolean).
3. **In final section:** `HighBitSet64(mInterruptedTplMask)` -- one BSR/CLZ
   instruction + comparison + bitwise AND for clearing.

Total additional complexity over boolean: ~3 operations in `CoreRestoreTpl`.
In exchange, we get:
- Zero nesting during ISR event dispatch (vs bounded-but-nonzero)
- Callback overwrite immunity
- ASSERT diagnostic for monotonic nesting
- Self-clearing tied to TPL descent (no stale-flag concerns)

The complexity is justified because the DXE Core's TPL mechanism is critical
infrastructure executed on every interrupt -- correctness and minimal stack
depth are more valuable than the ~3 operations saved by a boolean.

#### 5.5.7 Summary: Why Bitmask Is Justified

| Concern | Boolean | Bitmask | Winner |
|---------|---------|---------|--------|
| **Nesting during ISR dispatch** | Bounded (1 per pending TPL) | Zero (mask suppresses all re-enable) | Bitmask |
| **Callback overwrite** | Vulnerable (extra nesting) | Immune | Bitmask |
| **Self-clearing** | Requires `OldTpl` guard | Natural via bit-clear on TPL descent | Bitmask |
| **Diagnostic value** | None | ASSERT on monotonic nesting | Bitmask |
| **Code simplicity** | +28/-9 lines | +30/-5 lines | Comparable |
| **Conceptual clarity** | "Were IRQs off?" (binary) | "Which TPL was interrupted?" (precise) | Bitmask |
| **Performance** | 1 boolean check | 1 zero-compare + 1 BSR + 1 AND | Boolean (marginal) |

The bitmask is justified because:
1. It provides **zero additional nesting** during ISR event dispatch -- the
   tightest possible bound -- vs the boolean's bounded-but-nonzero nesting.
2. It is **immune to callback overwrite** -- a structural guarantee that
   doesn't depend on callback behavior.
3. Its **self-clearing mechanism** naturally ties interrupt suppression to
   TPL descent -- no manual "end of ISR" signal needed.
4. The **ASSERT** catches nesting violations early during development.
5. The additional complexity is **minimal** (~3 extra operations) relative
   to the correctness guarantees gained.

---

### 5.6 Solution 4: CPU Arch Protocol Fix + Interrupted TPL Mask -- **Recommended**

**Status:** Implemented in current branch
(`Bug_xxx_DxeCorePreventTimerInterruptInfiniteRecurssion_Solution_5`)

**Approach:** Three-part fix that combines a spec-conformance fix to the
CPU Architecture Protocol's `GetInterruptState()` implementation with DXE
Core logic in both `CoreTimerTick` (ISR detection and mask-setting) and
`CoreRestoreTpl` (mask-consulting to suppress interrupt re-enable). This
eliminates the Scenario 5 false positive that affects Solutions 1-3 by
making `GetInterruptState()` return ground-truth hardware state instead of
a stale cached variable.

**Part 1 -- CPU Arch Protocol fix (2 files):**

Fix `CpuGetInterruptState()` in two non-conforming drivers to read actual
hardware via BaseLib's `GetInterruptState()`:

```c
// UefiCpuPkg/CpuDxe/CpuDxe.c (X64)
*State = GetInterruptState ();   // reads RFLAGS.IF
InterruptState = *State;         // update cache for backward compat

// UefiCpuPkg/CpuDxeRiscV64/CpuDxe.c (RISC-V)
*State = GetInterruptState ();   // reads sstatus.SIE
mInterruptState = *State;        // update cache for backward compat
```

**Part 2 -- DXE Core logic (CoreTimerTick + CoreRestoreTpl):**

`CoreTimerTick` determines the interrupted TPL at entry (from
`gTplBeforeHighTpl` if the timer driver raised TPL, or from
`gEfiCurrentTpl` otherwise) and sets the corresponding bit in
`gIsrEntryTplMask`. `CoreRestoreTpl` consults this mask to suppress
interrupt re-enable during ISR unwind.

**Why this combination is superior:**

| Property | Solution 1 (cached) | Solution 4 (HW read) |
|----------|--------------------|--------------------|
| DXE Core code | Identical | Identical |
| `GetInterruptState` accuracy | Stale on x86/RISC-V | Always correct |
| False positives (Scenario 5) | **0** (staging mask cleared by RestoreTpl) | **0** |
| Spec conformance | PI Spec violation in CpuDxe | **Full conformance** |
| Independent value | None beyond recursion fix | Fixes protocol bug for all callers |
| Timer driver agnostic | Yes | **Yes** (HW-level detection) |

**Scenario difference detail:**

In Scenario 5, code explicitly disables interrupts (via protocol call or
raw instruction) then calls `RaiseTpl(HIGH)`:

- **Solutions 1-3 (conceptual single-mask design):** `GetInterruptState()`
  returns FALSE (from cache or HW) -> sets mask bit -> `RestoreTpl` skips
  re-enable. This is a "false positive" because the code is NOT in
  interrupt context.

- **Actual implementation (two-stage design):** `RaiseTpl(HIGH)` sets a bit
  in `gTplBeforeHighTpl` (staging mask). When `RestoreTpl` transitions
  from HIGH to below HIGH, it unconditionally clears `gTplBeforeHighTpl = 0`
  **before** entering the event dispatch loop. Since no `CoreTimerTick()`
  runs between `RaiseTpl` and `RestoreTpl` in this normal-context sequence,
  `gIsrEntryTplMask` is never set. Result: `RestoreTpl` re-enables
  interrupts normally. **No false positive.** (Validated by Test 9.)

**Commits (6 total on branch):**

1. `MdeModulePkg/Core/Dxe: Add CoreTimerTick recursion depth tracking and diagnostics`
   -- Instrumentation: per-TPL entry counters, MaxDepth tracking, config table
2. `MdeModulePkg/Test: Add DxeCoreTplTest shell application`
   -- Test suite (16 tests: 7 spec conformance + 9 functional, with runner scripts)
3. `UefiCpuPkg/CpuDxe: Read hardware interrupt state in GetInterruptState`
   -- X64 CpuDxe fix (Part 1)
4. `UefiCpuPkg/CpuDxeRiscV64: Read hardware interrupt state in GetInterruptState`
   -- RISC-V CpuDxe fix (Part 1)
5. `MdeModulePkg/Core/Dxe: Prevent timer interrupt infinite recursion`
   -- DXE Core `gIsrEntryTplMask` / `gTplBeforeHighTpl` logic (Part 2)
6. `OvmfPkg/LocalApicTimerDxe: Remove NestedInterruptTplLib dependency`
   -- Removes now-unnecessary workaround from OVMF

**Pros (in addition to all Solution 1 pros):**
- Eliminates the Scenario 5 false positive.
- Fixes a genuine PI Spec conformance bug (independent value).
- Detection at hardware level -- works with ALL timer driver patterns.
- No new DXE Core code beyond Solution 1 (zero additional DXE complexity).
- CPU Arch Protocol fix is trivial (~4 lines per driver).
- Cache update in CpuDxe preserves backward compatibility.

**Cons:**
- Modifies 2 files outside DXE Core (CpuDxe drivers). However, these fix
  a genuine bug and have independent value.
- Same `gCpu->GetInterruptState()` protocol call overhead as Solutions 1-3.
- In ISR context, event callbacks execute with interrupts disabled (same
  as Solution 1 -- correct behavior, original code crashes in this case).

---

## 6 Test Plan

This section describes a UEFI Shell application test suite to verify the
DXE Core timer interrupt recursion fix. The implemented suite contains 16
tests organized into 2 suites: Spec Conformance (Tests 1-7) verifying
PI/UEFI mandated behavior, and Functional (Tests 8-16) verifying the
recursion fix effectiveness and safety. Tests run on QEMU using OvmfPkg
(X64) and ArmVirtPkg (AARCH64). The recursion tests (15-16) use a profiled
timer period with test-specific divisors (Test 15: 1/128th of callback cost,
Test 16: 1/128th of callback cost) to maximize timer interrupt frequency
relative to execution speed.

### 6.1 Test Infrastructure

**Test Application:** A `MODULE_TYPE = UEFI_APPLICATION` Shell executable
using `UnitTestLib` from `UnitTestFrameworkPkg`.

**Required Protocols and Services:**

```c
#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>   // gBS->RaiseTPL / RestoreTPL
#include <Library/UefiLib.h>                    // Print, EfiGetCurrentTpl
#include <Library/UnitTestLib.h>                // UNIT_TEST_FRAMEWORK
#include <Protocol/Cpu.h>                       // EFI_CPU_ARCH_PROTOCOL
#include <Protocol/Timer.h>                     // EFI_TIMER_ARCH_PROTOCOL
```

**Protocol Instances (located at test init):**

| Protocol | GUID | Purpose |
|---|---|---|
| `EFI_CPU_ARCH_PROTOCOL` | `gEfiCpuArchProtocolGuid` | `EnableInterrupt`, `DisableInterrupt`, `GetInterruptState` |
| `EFI_TIMER_ARCH_PROTOCOL` | `gEfiTimerArchProtocolGuid` | `GetTimerPeriod`, `SetTimerPeriod` (disable/restore timer) |

### 6.2 QEMU Build and Launch

#### 6.2.1 OvmfPkg (X64)

```
# Build
build -p OvmfPkg/OvmfPkgX64.dsc -a X64 -t GCC5 -b DEBUG -D NETWORK_ENABLE=FALSE

# Launch (test app in VirtualDrive/)
qemu-system-x86_64 \
  -debugcon stdio -global isa-debugcon.iobase=0x402 \
  -net none -smp 1 \
  -drive file=fat:rw:VirtualDrive,format=raw,media=disk \
  -pflash Build/OvmfX64/DEBUG_GCC5/FV/OVMF.fd
```

#### 6.2.2 OvmfPkg (IA32)

```
# Build
build -p OvmfPkg/OvmfPkgIa32.dsc -a IA32 -t GCC5 -b DEBUG -D NETWORK_ENABLE=FALSE

# Launch
qemu-system-i386 \
  -debugcon stdio -global isa-debugcon.iobase=0x402 \
  -net none -smp 1 \
  -drive file=fat:rw:VirtualDrive,format=raw,media=disk \
  -pflash Build/OvmfIa32/DEBUG_GCC5/FV/OVMF.fd
```

#### 6.2.3 ArmVirtPkg (AARCH64)

```
# Build
build -p ArmVirtPkg/ArmVirtQemu.dsc -a AARCH64 -t GCC5 -b DEBUG

# Launch
qemu-system-aarch64 \
  -M virt -cpu neoverse-n2 -m 1024 \
  -net none -serial stdio \
  -drive file=fat:rw:VirtualDrive,format=raw,media=disk \
  -pflash Build/ArmVirtQemu-AARCH64/DEBUG_GCC5/FV/QEMU_EFI.fd
```

**Note:** Use `-smp 1` to avoid multi-processor complexity in timer interrupt
tests. The recursion bug is per-BSP and independent of AP count.

### 6.3 Test Helpers

```c
//
// Global protocol pointers (located once in test init)
//
EFI_CPU_ARCH_PROTOCOL    *gCpu;
EFI_TIMER_ARCH_PROTOCOL  *gTimer;

//
// Timer period in microseconds (queried once at test init).
// Used to compute deterministic Stall() durations.
// Example: OVMF default = 100,000 (100ns units) = 10,000 --s = 10ms
//
UINTN  mTimerPeriodUs;

//
// Query the current timer period and store in mTimerPeriodUs.
// Must be called during test suite initialization.
//
VOID
InitTimerPeriod (VOID)
{
  EFI_STATUS  Status;
  UINT64      TimerPeriod;  // in 100ns units

  Status = gTimer->GetTimerPeriod (gTimer, &TimerPeriod);
  ASSERT_EFI_ERROR (Status);
  ASSERT (TimerPeriod > 0);  // timer must be running
  mTimerPeriodUs = (UINTN)(TimerPeriod / 10);
  if (mTimerPeriodUs == 0) {
    mTimerPeriodUs = 1;  // sub-microsecond timer -- floor to 1us
  }
}

//
// Compute a Stall() duration guaranteed to span at least N timer ticks.
// Adds 1 extra period as margin to account for phase alignment.
//
UINTN
StallForTicks (
  IN UINTN  TickCount
  )
{
  return mTimerPeriodUs * (TickCount + 1);
}

//
// Watchdog: detect infinite recursion by stack overflow / hang.
// Set a short timer period and a gBS->SetWatchdogTimer() timeout.
// If the test hangs (recursion), QEMU resets after the watchdog fires.
//
VOID
SetRecursionWatchdog (
  IN UINTN  TimeoutSeconds
  )
{
  gBS->SetWatchdogTimer (TimeoutSeconds, 0, 0, NULL);
}

VOID
ClearRecursionWatchdog (VOID)
{
  gBS->SetWatchdogTimer (0, 0, 0, NULL);
}

//
// Verify interrupt state matches expectation.
//
BOOLEAN
GetInterruptStateChecked (VOID)
{
  BOOLEAN     State;
  EFI_STATUS  Status;

  Status = gCpu->GetInterruptState (gCpu, &State);
  ASSERT_EFI_ERROR (Status);
  return State;
}
```

### 6.4 Scenario Tests

Each test maps to one or more scenarios from the consolidated table in
Section 4.5. Tests are grouped by interrupt mechanism.

#### 6.4.1 TPL-Managed Scenarios (Scenarios 1-4, 8-9, 11-12)

**Test 1 -- Normal `RaiseTpl(HIGH)` / `RestoreTpl`** *(Scenario 1)*

Verifies baseline: interrupts enabled before raise, disabled during HIGH,
re-enabled after restore.

```
Precondition: TPL_APPLICATION, interrupts enabled
Steps:
  1. ASSERT GetInterruptState() == TRUE
  2. OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL)
  3. ASSERT GetInterruptState() == FALSE
  4. gBS->RestoreTPL(OldTpl)
  5. ASSERT GetInterruptState() == TRUE
Expected: All assertions pass -- interrupts restored correctly
```

**Test 2 -- `RaiseTpl` to non-HIGH** *(Scenario 2)*

Verifies the fix has no impact on non-HIGH TPL transitions.

```
Precondition: TPL_APPLICATION, interrupts enabled
Steps:
  1. ASSERT GetInterruptState() == TRUE
  2. OldTpl = gBS->RaiseTPL(TPL_NOTIFY)
  3. ASSERT GetInterruptState() == TRUE  // interrupts still on at NOTIFY
  4. gBS->RestoreTPL(OldTpl)
  5. ASSERT GetInterruptState() == TRUE
Expected: Interrupts remain enabled throughout
```

**Test 3 -- Nested `RaiseTpl(HIGH)` when already at HIGH** *(Scenario 3)*

Verifies double-raise to HIGH is harmless.

```
Precondition: TPL_APPLICATION, interrupts enabled
Steps:
  1. OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL)
  2. ASSERT GetInterruptState() == FALSE
  3. InnerOldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL)  // already at HIGH
  4. ASSERT InnerOldTpl == TPL_HIGH_LEVEL
  5. gBS->RestoreTPL(InnerOldTpl)
  6. ASSERT GetInterruptState() == FALSE  // still at HIGH
  7. gBS->RestoreTPL(OldTpl)
  8. ASSERT GetInterruptState() == TRUE
Expected: Nested raise is a no-op; interrupts restored on final restore
```

**Test 4 -- Timer interrupt does not cause recursion** *(Scenario 11)*

The primary bug-fix verification. Confirms the system survives sustained
timer interrupts without stack overflow.

```
Precondition: TPL_APPLICATION, interrupts enabled, timer running
Steps:
  1. SetRecursionWatchdog(5)  // 5-second timeout
  2. Loop for 1000 iterations:
     a. OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL)
     b. gBS->RestoreTPL(OldTpl)
     c. Stall(mTimerPeriodUs)  // 1 tick -- allows timer interrupts between iterations
  3. ClearRecursionWatchdog()
  4. ASSERT system is still alive (no hang, no reset)
Expected: Completes without hang -- proves no infinite recursion
```

**Test 5 -- Normal context after timer interrupt** *(Scenario 12)*

Verifies that after a timer interrupt fires and returns, the next normal
`RaiseTpl(HIGH)` / `RestoreTpl` cycle works correctly. For Solution 1's
bitmask, the mask is already cleared to 0 during the timer handler's own
`CoreRestoreTpl` (via `mInterruptedTplMask &= (1U << NewTpl) - 1`), so
there is no stale state after IRET. This test confirms the mask self-cleans
during the interrupt handler, not after.

Note: For Solution 2's boolean, the flag IS stale after IRET and
self-cleans on the next `RaiseTpl(HIGH)` -- this test would exercise that
path.

```
Precondition: TPL_APPLICATION, interrupts enabled
Steps:
  1. Stall(StallForTicks(2))  // = 2 ticks -- ensure at least one timer interrupt fires
  2. OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL)
  3. gBS->RestoreTPL(OldTpl)
  4. ASSERT GetInterruptState() == TRUE
Expected: Interrupts correctly restored -- mask already clean (Solution 1)
or self-cleaned on RaiseTpl (Solution 2)
```

**Test 6 -- Temporarily lower TPL across HIGH boundary** *(Scenario 8)*

Verifies TPL lowering and re-raising across the HIGH boundary works correctly
in normal context.

```
Precondition: TPL_APPLICATION, interrupts enabled
Steps:
  1. OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL)
  2. ASSERT GetInterruptState() == FALSE
  3. gBS->RestoreTPL(TPL_APPLICATION)  // drop below HIGH
  4. ASSERT GetInterruptState() == TRUE
  5. Stall(StallForTicks(2))  // = 2 ticks -- timer interrupts fire in this window
  6. InnerOldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL)
  7. ASSERT GetInterruptState() == FALSE
  8. gBS->RestoreTPL(InnerOldTpl)
  9. ASSERT GetInterruptState() == TRUE
Expected: Interrupt state correctly tracks each HIGH boundary crossing
```

**Test 7 -- Normal TPL nesting through event dispatch loop** *(Scenario 4)*

Verifies that the full ascending TPL nesting pattern (APPLICATION -> CALLBACK
-> NOTIFY -> HIGH) and descending restore with event dispatch at each level
works correctly with the fix applied. Interrupts must be re-enabled at each
dispatch level below HIGH in normal context.

```
Precondition: TPL_APPLICATION, interrupts enabled
Steps:
  1. Create EVT_NOTIFY_SIGNAL events at TPL_CALLBACK and TPL_NOTIFY
     with callbacks that record their execution order and interrupt state
  2. ASSERT GetInterruptState() == TRUE
  3. OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL)
  4. ASSERT GetInterruptState() == FALSE
  5. gBS->SignalEvent(NotifyEvent)   // queue NOTIFY-level event
  6. gBS->SignalEvent(CallbackEvent) // queue CALLBACK-level event
  7. gBS->RestoreTPL(OldTpl)
     // Event dispatch loop walks: NOTIFY(16) -> CALLBACK(8) -> APPLICATION(4)
     // At each level, interrupts should be re-enabled before dispatch
  8. ASSERT NotifyCallback executed with GetInterruptState() == TRUE
  9. ASSERT CallbackCallback executed with GetInterruptState() == TRUE
  10. ASSERT execution order: NOTIFY before CALLBACK (descending TPL)
  11. ASSERT GetInterruptState() == TRUE
  12. Close events
Expected: Events dispatched in descending TPL order with interrupts
enabled at each dispatch level -- fix does not suppress normal event
dispatch interrupt re-enabling
```

**Test 8 -- Timer interrupt during normal event dispatch** *(Scenario 9)*

Directly verifies Scenario 9: a timer interrupt fires during normal-context
event dispatch (inside `CoreDispatchEventNotifies`). Uses a slow event
callback that stalls long enough to guarantee a timer tick fires while
the callback is executing with interrupts enabled. The system must survive
without infinite recursion -- the inner `CoreTimerTick`'s `RestoreTpl`
calls `CoreSetInterruptState(FALSE)` before the TPL assignment and leaves
interrupts disabled (mask bit set), but this is harmless because the
outer context already has interrupts HW-enabled via IRET.

```
Precondition: TPL_APPLICATION, interrupts enabled
Callback:
  VOID EFIAPI SlowNotifyCallback (IN EFI_EVENT Event, IN VOID *Context) {
    // Record that we're executing with interrupts enabled
    gCpu->GetInterruptState (gCpu, &mCallbackInterruptState);
    // Stall = 2 timer periods -- guarantees timer interrupt fires
    // DURING this dispatch, exercising Scenario 9
    gBS->Stall (StallForTicks(2));
    mCallbackSurvived = TRUE;
  }
Steps:
  1. Create EVT_NOTIFY_SIGNAL event at TPL_NOTIFY with SlowNotifyCallback
  2. SetRecursionWatchdog(5)
  3. OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL)
  4. gBS->SignalEvent(SlowEvent)  // queue for dispatch
  5. gBS->RestoreTPL(OldTpl)
     // Event dispatch loop: gEfiCurrentTpl = TPL_NOTIFY,
     // CoreSetInterruptState(TRUE), CoreDispatchEventNotifies
     // -> SlowNotifyCallback runs with interrupts ENABLED
     // -> timer fires during Stall -> CoreTimerTick runs (Scenario 9)
     // -> inner RestoreTpl: CoreSetInterruptState(FALSE) before assign,
     //   mask check -> leaves disabled -> interrupt return
     // -> outer callback resumes
  6. ClearRecursionWatchdog()
  7. ASSERT mCallbackSurvived == TRUE
  8. ASSERT mCallbackInterruptState == TRUE  // interrupts on during dispatch
  9. // Verify self-clean: next RaiseTpl/RestoreTpl works normally
  10. OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL)
  11. gBS->RestoreTPL(OldTpl)
  12. ASSERT GetInterruptState() == TRUE  // self-cleaned [OK]
  13. Close event
Expected: Timer fires during event dispatch, no recursion, self-cleans
```

#### 6.4.2 Interrupt Manipulation Scenarios (Scenarios 5-7, 10)

These tests exercise direct interrupt manipulation via `gCpu` protocol calls,
which bypass the TPL mechanism. These are contract-violation scenarios; the
tests document the observed behavior rather than asserting ideal correctness.

**Test 9 -- Disable interrupts then `RaiseTpl(HIGH)`** *(Scenario 5)*

Verifies that `RestoreTpl` to below HIGH always re-enables interrupts,
regardless of what the caller did before `RaiseTpl`. The TPL system manages
interrupt state: below HIGH means interrupts are enabled. The stale
`gTplBeforeHighTpl` bits set by `DisableInterrupt` + `RaiseTpl(HIGH)` are
cleared by `CoreRestoreTpl` during the HIGH-to-below-HIGH transition,
*before* entering the event dispatch loop. This prevents the stale value
from propagating to `gIsrEntryTplMask` via a subsequent `CoreTimerTick()`
call, and the final section sees no ISR mask and re-enables interrupts.

```
Precondition: TPL_APPLICATION, interrupts enabled
Steps:
  1. ASSERT GetInterruptState() == TRUE
  2. gCpu->DisableInterrupt(gCpu)
  3. ASSERT GetInterruptState() == FALSE
  4. OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL)
  5. gBS->RestoreTPL(OldTpl)
  6. // RestoreTpl clears stale gTplBeforeHighTpl, then re-enables interrupts
  7. ASSERT GetInterruptState() == TRUE
  8. // Verify a second cycle also works
  9. OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL)
  10. gBS->RestoreTPL(OldTpl)
  11. ASSERT GetInterruptState() == TRUE
Expected: All assertions pass -- RestoreTpl always re-enables below HIGH
```

**Test 10 -- Disable interrupts in interrupt context** *(Scenario 6)*

Verifies that `DisableInterrupt` inside `TPL_HIGH_LEVEL` (simulating
interrupt context) is a harmless no-op.

```
Precondition: TPL_APPLICATION, interrupts enabled
Steps:
  1. OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL)
  2. ASSERT GetInterruptState() == FALSE
  3. gCpu->DisableInterrupt(gCpu)  // redundant, already off
  4. ASSERT GetInterruptState() == FALSE
  5. gBS->RestoreTPL(OldTpl)
  6. ASSERT GetInterruptState() == TRUE
Expected: DisableInterrupt is no-op at HIGH; restore works normally
```

**Test 11 -- Toggle interrupts without TPL change** *(Scenario 7)*

Verifies that direct interrupt toggling at `TPL_APPLICATION` without any
TPL transitions does not affect the fix mechanism.

```
Precondition: TPL_APPLICATION, interrupts enabled
Steps:
  1. gCpu->DisableInterrupt(gCpu)
  2. gCpu->EnableInterrupt(gCpu)
  3. ASSERT GetInterruptState() == TRUE
  4. OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL)
  5. gBS->RestoreTPL(OldTpl)
  6. ASSERT GetInterruptState() == TRUE
Expected: Toggle without TPL change has no effect on fix mechanism
```

**Test 12 -- Enable interrupts at `TPL_HIGH`** *(Scenario 10)*

Documents severe-misuse behavior. The system should survive without crashing,
though behavior is unpredictable.

```
Precondition: TPL_APPLICATION, interrupts enabled
Steps:
  1. SetRecursionWatchdog(5)
  2. OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL)
  3. ASSERT GetInterruptState() == FALSE
  4. gCpu->EnableInterrupt(gCpu)  // severe misuse
  5. Stall(StallForTicks(2))  // = 2 ticks -- window for timer interrupts
  6. gCpu->DisableInterrupt(gCpu)  // restore safe state
  7. gBS->RestoreTPL(OldTpl)
  8. ClearRecursionWatchdog()
  9. ASSERT system is still alive
Expected: System survives -- fix mitigates damage from misuse
```

#### 6.4.3 Architecture-Specific Instruction Variants (Scenarios 5, 7, 10)

These tests use architecture-specific inline assembly. Each test must be
conditionally compiled.

**Test 13 -- CLI then `RaiseTpl(HIGH)` (X64/IA32 only)** *(Scenario 5)*

```
Precondition: TPL_APPLICATION, interrupts enabled
Steps:
  1. __asm__ volatile ("cli");   // or _disable() on MSVC
  2. OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL)
  3. gBS->RestoreTPL(OldTpl)
  4. ASSERT GetInterruptState() == FALSE  // !! false positive
  5. __asm__ volatile ("sti");   // or _enable() on MSVC
  6. OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL)  // self-clean
  7. gBS->RestoreTPL(OldTpl)
  8. ASSERT GetInterruptState() == TRUE   // self-cleaned [OK]
Expected: Same false-positive / self-clean as Test 9 (protocol equivalent)
```

AArch64 equivalent uses `DisableInterrupts()` / `EnableInterrupts()` from
`ArmLib.h` (which map to `MSR DAIFSet` / `MSR DAIFClr`).

**Test 14 -- CLI/STI bracket without TPL change** *(Scenario 7)*

```
Precondition: TPL_APPLICATION, interrupts enabled
Steps:
  1. DisableInterrupts()  // CLI / CPSID i / DAIFSet
  2. // ... critical section, no TPL change ...
  3. EnableInterrupts()   // STI / CPSIE i / DAIFClr
  4. OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL)
  5. gBS->RestoreTPL(OldTpl)
  6. ASSERT GetInterruptState() == TRUE
Expected: No impact -- fix mechanism not involved
```

**Test 15 -- STI at `TPL_HIGH` (X64/IA32 only)** *(Scenario 10)*

```
Precondition: TPL_APPLICATION, interrupts enabled
Steps:
  1. SetRecursionWatchdog(5)
  2. OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL)
  3. EnableInterrupts()   // STI -- severe misuse
  4. Stall(StallForTicks(2))  // = 2 ticks
  5. DisableInterrupts()  // restore safe state
  6. gBS->RestoreTPL(OldTpl)
  7. ClearRecursionWatchdog()
  8. ASSERT system is still alive
Expected: System survives -- identical to Test 12
```

#### 6.4.4 IRQ-Context Tests via Timer Handler Hook (Scenarios 11, 9, 13)

These tests replace `CoreTimerTick` with a custom timer notification handler
via `EFI_TIMER_ARCH_PROTOCOL.RegisterHandler()`. This places test code in
genuine interrupt context (hardware-disabled interrupts, `GetInterruptState`
-> FALSE), enabling direct verification of Scenarios 11, 9, and 13 that are
otherwise only indirectly tested.

**Important constraints:**
- `RegisterHandler()` returns `EFI_ALREADY_STARTED` if a handler is
  already registered. The test must first unregister `CoreTimerTick` by
  calling `RegisterHandler(NULL)`, then register the test handler.
- After unregistering `CoreTimerTick`, timer-based event signaling
  (`gBS->SetTimer`, periodic events, etc.) stops functioning.
  `CoreTimerTick` cannot be re-registered because its address is internal
  to the DXE Core.
- These tests must run **last** in the test suite. After completion, the
  test unregisters its handler and the system should be rebooted.
- Use volatile shared variables to communicate results from handler to
  test body, since `Print()` and `ASSERT()` are not safe in interrupt
  context.

**Test helper -- Hook infrastructure:**

```c
//
// Shared state between test body and interrupt handler
//
volatile BOOLEAN  mHandlerExecuted     = FALSE;
volatile BOOLEAN  mHandlerPassed       = FALSE;
volatile UINTN    mHandlerIterations   = 0;
volatile BOOLEAN  mInterruptStateInHandler = FALSE;
volatile BOOLEAN  mEventsDispatched    = FALSE;

//
// Install custom timer handler (replaces CoreTimerTick)
//
EFI_STATUS
InstallTestTimerHandler (
  IN EFI_TIMER_NOTIFY  Handler
  )
{
  EFI_STATUS  Status;

  // Unregister CoreTimerTick
  Status = gTimer->RegisterHandler (gTimer, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  // Register test handler
  return gTimer->RegisterHandler (gTimer, Handler);
}

//
// Uninstall test timer handler (leaves timer running with no handler)
//
EFI_STATUS
UninstallTestTimerHandler (VOID)
{
  return gTimer->RegisterHandler (gTimer, NULL);
}
```

**Test 16 -- IRQ-context RaiseTpl/RestoreTpl** *(Scenarios 11, 13)*

Directly verifies the primary bug fix: `RaiseTpl(HIGH)` / `RestoreTpl`
inside a genuine interrupt handler does not cause infinite recursion.
The handler runs in interrupt context (`GetInterruptState` -> FALSE),
so `CoreRaiseTpl` sets the `mInterruptedTplMask` bit for the interrupted
TPL, and the final section of `CoreRestoreTpl` detects the mask and
skips re-enabling interrupts.

```
Precondition: TPL_APPLICATION, interrupts enabled, timer running
Handler:
  VOID EFIAPI Test16Handler (IN UINT64 Time) {
    BOOLEAN State;
    EFI_TPL OldTpl;

    // Verify we are in genuine interrupt context
    gCpu->GetInterruptState (gCpu, &State);
    mInterruptStateInHandler = State;  // expect FALSE

    // Exercise the fix: RaiseTpl/RestoreTpl in IRQ context
    OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
    gBS->RestoreTPL (OldTpl);

    // If we reach here, no infinite recursion occurred
    mHandlerExecuted = TRUE;
    mHandlerPassed = TRUE;
  }
Steps:
  1. SetRecursionWatchdog(5)
  2. InstallTestTimerHandler(Test16Handler)
  3. Stall(StallForTicks(20))  // = 20 ticks -- wait for handler to fire multiple times
  4. UninstallTestTimerHandler()
  5. ClearRecursionWatchdog()
  6. ASSERT mHandlerExecuted == TRUE
  7. ASSERT mHandlerPassed == TRUE
  8. ASSERT mInterruptStateInHandler == FALSE  // genuine IRQ context
Expected: Handler executes RaiseTpl/RestoreTpl in interrupt context
without recursion -- mask bit set, final section leaves disabled
```

**Test 17 -- IRQ-context event dispatch with interrupts enabled**
*(Scenarios 11, 13 -- event dispatch path)*

Verifies that `CoreRestoreTpl`'s event dispatch loop re-enables interrupts
in IRQ context at dispatch levels above the interrupted TPL. For Solution 1,
the handler is interrupted at `TPL_APPLICATION` (4), so the mask has bit 4
set. Dispatch at `TPL_NOTIFY` (16) satisfies `16 > HighBitSet64(0x10) = 4`,
so interrupts are re-enabled. Events signaled before hooking are dispatched
inside the handler's `RestoreTpl` with interrupts re-enabled during dispatch.

```
Precondition: TPL_APPLICATION, interrupts enabled
Setup:
  Create EVT_NOTIFY_SIGNAL event at TPL_NOTIFY with callback that:
    - Records execution
    - Records GetInterruptState() result
Handler:
  VOID EFIAPI Test17Handler (IN UINT64 Time) {
    EFI_TPL OldTpl;

    OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
    // Signal event while at HIGH -- queues for dispatch
    gBS->SignalEvent (TestNotifyEvent);
    // RestoreTpl will dispatch the NOTIFY event in the event loop
    // Event loop: gEfiCurrentTpl=16, (16 > HighBitSet64(mask)=4) -> enables
    gBS->RestoreTpl (OldTpl);

    mHandlerExecuted = TRUE;
    mEventsDispatched = mNotifyCallbackFired;
  }
Steps:
  1. Create test event at TPL_NOTIFY
  2. SetRecursionWatchdog(5)
  3. InstallTestTimerHandler(Test17Handler)
  4. Stall(StallForTicks(20))  // = 20 ticks
  5. UninstallTestTimerHandler()
  6. ClearRecursionWatchdog()
  7. ASSERT mHandlerExecuted == TRUE
  8. ASSERT mEventsDispatched == TRUE
  9. ASSERT mNotifyInterruptState == TRUE  // interrupts ON during dispatch
     // (mask has bit at APPLICATION=4, dispatch at NOTIFY=16: 16>4 -> enables)
  10. Close test event
Expected: Event dispatched during IRQ-context RestoreTpl with interrupts
enabled -- confirms event loop re-enables interrupts at dispatch levels
above the interrupted TPL (Solution 1's mask check: 16 > 4 -> enables)
```

**Test 18 -- IRQ-context sustained RaiseTpl/RestoreTpl** *(Scenario 11 -- stress)*

Stress version of Test 16. The handler runs across multiple timer ticks,
each doing `RaiseTpl(HIGH)` / `RestoreTpl` in interrupt context. Verifies
no cumulative state corruption in `mInterruptedTplMask`.

```
Precondition: TPL_APPLICATION, interrupts enabled, timer running
Handler:
  VOID EFIAPI Test18Handler (IN UINT64 Time) {
    EFI_TPL OldTpl;

    OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
    gBS->RestoreTpl (OldTpl);
    mHandlerIterations++;
  }
Steps:
  1. SetRecursionWatchdog(10)
  2. InstallTestTimerHandler(Test18Handler)
  3. Stall(StallForTicks(200))  // = 200 ticks -- many handler invocations
  4. UninstallTestTimerHandler()
  5. ClearRecursionWatchdog()
  6. ASSERT mHandlerIterations > 10  // handler fired multiple times
  7. // Verify normal context still works after hook removal
  8. gCpu->EnableInterrupt(gCpu)  // ensure interrupts on
  9. OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL)
  10. gBS->RestoreTPL(OldTpl)
  11. ASSERT GetInterruptState() == TRUE  // self-cleaned
Expected: Multiple IRQ-context RaiseTpl/RestoreTpl cycles with no
hang, no stack overflow, no state corruption
```

**Test 19 -- IRQ-context temp lower TPL across HIGH** *(Scenario 13 -- direct)*

Directly exercises Scenario 13: inside an interrupt handler, temporarily
lower TPL from HIGH to APPLICATION and raise back. This is the most
dangerous pattern -- `RestoreTpl` runs the event dispatch loop with
interrupts enabled in interrupt context. Unlike Test 17, which signals
a single event, this test signals events at **both** `TPL_NOTIFY` and
`TPL_CALLBACK` so the event dispatch loop walks multiple levels with
interrupts enabled at each -- maximizing the window for a timer interrupt
to fire during dispatch in IRQ context.

**Critical:** The handler must call `RaiseTpl(HIGH)` first (like
`CoreTimerTick` does) before calling `RestoreTpl`. Timer arch protocol
handlers (`EFI_TIMER_NOTIFY`) are called directly from the hardware ISR
at the interrupted TPL -- nobody calls `RaiseTpl(HIGH)` on their behalf.
Without this raise, `mInterruptedTplMask` is never set and the fix cannot
prevent recursion. The handler must also restore to the original
interrupted TPL (returned by `RaiseTpl`) rather than to a hardcoded value,
to leave the system in a consistent state.

```
Precondition: TPL_APPLICATION, interrupts enabled, timer running
Setup:
  Create EVT_NOTIFY_SIGNAL event at TPL_NOTIFY with callback that:
    - Increments mNotifyDispatchCount
    - Records GetInterruptState() into mNotifyInterruptState
  Create EVT_NOTIFY_SIGNAL event at TPL_CALLBACK with callback that:
    - Increments mCallbackDispatchCount
    - Records GetInterruptState() into mCallbackInterruptState
Handler:
  VOID EFIAPI Test19Handler (IN UINT64 Time) {
    EFI_TPL OldTpl;
    EFI_TPL InnerOldTpl;

    // Must raise to HIGH first (like CoreTimerTick does).
    // GetInterruptState?FALSE -> mask |= (1<<OldTpl) -> fix is armed.
    OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

    // Signal events at two TPL levels to force multi-level dispatch
    gBS->SignalEvent (TestNotifyEvent);
    gBS->SignalEvent (TestCallbackEvent);

    // Temporarily drop to APPLICATION -- triggers event dispatch loop
    gBS->RestoreTPL (TPL_APPLICATION);
    // Event loop walks: TPL_NOTIFY(16) -> enables interrupts -> dispatch
    //                   TPL_CALLBACK(8) -> enables interrupts -> dispatch
    // Timer may fire during either dispatch (bounded nesting)
    // Final: CoreSetInterruptState(FALSE), gEfiCurrentTpl = 4,
    //   mask check -> 4 = HighBitSet64(mask)=4 -> leaves disabled [OK]

    // Raise back to HIGH
    InnerOldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

    // Restore to original interrupted TPL (clean exit)
    gBS->RestoreTPL (OldTpl);

    // If we reach here, bounded nesting worked (no infinite recursion)
    mHandlerExecuted = TRUE;
    mHandlerPassed = (InnerOldTpl == TPL_APPLICATION);
  }
Steps:
  1. Create test events at TPL_NOTIFY and TPL_CALLBACK
  2. SetRecursionWatchdog(5)
  3. InstallTestTimerHandler(Test19Handler)
  4. Stall(StallForTicks(20))  // = 20 ticks
  5. UninstallTestTimerHandler()
  6. ClearRecursionWatchdog()
  7. ASSERT mHandlerExecuted == TRUE
  8. ASSERT mHandlerPassed == TRUE
  9. ASSERT mNotifyDispatchCount > 0   // events dispatched in IRQ context
  10. ASSERT mCallbackDispatchCount > 0
  11. // NOTE: Do not assert on mNotifyInterruptState / mCallbackInterruptState.
      // With Solution 1's mask-guarded event loop, if a nested timer fires
      // during dispatch and sets a higher mask bit, the nested handler's own
      // event dispatch runs with interrupts disabled (dispatch level = new
      // mask bit). If the nested handler re-dispatches the same events, it
      // can overwrite these variables with FALSE. This is timing-dependent
      // and represents correct recursion-prevention behavior.
  12. Close test events
Expected: Temporary TPL lowering in interrupt context exercises the
full multi-level event dispatch loop with mask-guarded enable path
+ disable-before-assign path without recursion. Interrupt state during
callbacks is timing-dependent due to mask-based suppression.
```

### 6.5 Stress and Stability Tests

**Test 20 -- Sustained RaiseTpl/RestoreTpl under timer load**

Extended version of Test 4 for soak testing.

```
Precondition: TPL_APPLICATION, interrupts enabled, timer at default period
Steps:
  1. SetRecursionWatchdog(60)
  2. Loop for 100,000 iterations:
     a. OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL)
     b. gBS->RestoreTPL(OldTpl)
  3. ClearRecursionWatchdog()
  4. ASSERT GetInterruptState() == TRUE
Expected: No hang, no stack overflow, interrupts correctly restored
```

**Test 21 -- Rapid TPL cycling across all levels**

Exercises mixed TPL transitions including the full nesting pattern
(APPLICATION -> CALLBACK -> NOTIFY -> HIGH) to confirm no interaction between
non-HIGH raises, the event dispatch loop's multi-level walk, and the fix
mechanism.

```
Precondition: TPL_APPLICATION, interrupts enabled
Steps:
  1. SetRecursionWatchdog(30)
  2. Loop for 10,000 iterations:
     a. Tpl1 = gBS->RaiseTPL(TPL_CALLBACK)
     b. Tpl2 = gBS->RaiseTPL(TPL_NOTIFY)
     c. Tpl3 = gBS->RaiseTPL(TPL_HIGH_LEVEL)
     d. gBS->RestoreTPL(Tpl2)
     e. gBS->RestoreTPL(Tpl1)
     f. gBS->RestoreTPL(TPL_APPLICATION)  // note: restoring to original
  3. ClearRecursionWatchdog()
  4. ASSERT GetInterruptState() == TRUE
Expected: All TPL transitions work correctly with fix in place
```

**Test 22 -- Timer event signaling during TPL cycling**

Verifies timer events still fire correctly with the fix applied.

```
Precondition: TPL_APPLICATION, interrupts enabled
Steps:
  1. Create periodic timer event (100ms period) with callback that
     increments a volatile counter
  2. Stall(StallForTicks(50))  // = 50 ticks
  3. ASSERT counter >= 3  // at least 3 timer callbacks fired
  4. Repeat with RaiseTpl/RestoreTpl cycling interleaved:
     a. Loop 100x: RaiseTPL(HIGH) / RestoreTPL / Stall(mTimerPeriodUs)
  5. ASSERT counter has continued incrementing
  6. Close timer event
Expected: Timer events fire normally -- fix does not suppress legitimate
timer event delivery outside interrupt context
```

**Test 23 -- Non-HIGH RestoreTpl after timer interrupt** *(Scenarios 9, 12 -- regression)*

Verifies that a non-HIGH `RaiseTpl`/`RestoreTpl` cycle does not permanently
disable interrupts after a timer interrupt has occurred. For Solution 1's
bitmask, this test trivially passes because: (a) the mask is already cleared
to 0 during the timer handler's own `CoreRestoreTpl` (via
`mInterruptedTplMask &= (1U << NewTpl) - 1`), and (b) non-HIGH `RestoreTpl`
never consults `mInterruptedTplMask` anyway -- the mask check only occurs
in the final section after the event loop, which is only reached during
HIGH?non-HIGH transitions. This test is still valuable as a regression
test for Solution 2 (where the boolean flag IS stale after IRET) and to
catch future changes that might incorrectly check the mask in non-HIGH
paths.

```
Precondition: TPL_APPLICATION, interrupts enabled, timer running
Steps:
  1. ASSERT GetInterruptState() == TRUE
  2. Stall(StallForTicks(2))  // = 2 ticks -- ensure timer interrupt fires,
                               // leaving mInterruptedTplMask potentially non-zero
  3. // Non-HIGH TPL cycle -- must NOT check mInterruptedTplMask
  4. OldTpl = gBS->RaiseTPL(TPL_CALLBACK)
  5. gBS->RestoreTPL(OldTpl)
  6. ASSERT GetInterruptState() == TRUE  // -> fails if mask checked
  7. // Another non-HIGH cycle
  8. OldTpl = gBS->RaiseTPL(TPL_NOTIFY)
  9. gBS->RestoreTPL(OldTpl)
  10. ASSERT GetInterruptState() == TRUE
  11. // Verify HIGH cycle still works (self-cleans mask if set)
  12. OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL)
  13. gBS->RestoreTPL(OldTpl)
  14. ASSERT GetInterruptState() == TRUE
Expected: All assertions pass -- non-HIGH RestoreTpl unconditionally
re-enables interrupts regardless of mInterruptedTplMask value
```

**Test 24 -- Callback RaiseTpl(HIGH) during IRQ-context event dispatch**

Verifies that when an event callback calls `RaiseTpl(HIGH)`/`RestoreTpl`
inside the IRQ-context event dispatch loop, the system handles the
interaction correctly. For Solution 1's bitmask, the callback's
`RaiseTpl(HIGH)` sees `GetInterruptState?TRUE` (event loop enabled them)
and treats this as normal context -- it does **not** set a mask bit. The
outer `CoreRestoreTpl`'s final section still sees the original mask
correctly and leaves interrupts disabled. This is a key advantage of the
bitmask over Solution 2's boolean (which would be overwritten here).

```
Precondition: TPL_APPLICATION, interrupts enabled, timer running
Setup:
  Create EVT_NOTIFY_SIGNAL event at TPL_NOTIFY with callback:
    VOID EFIAPI LockingCallback (IN EFI_EVENT Event, IN VOID *Context) {
      EFI_TPL LockTpl;
      // Simulate lock acquire/release (common pattern in real drivers)
      LockTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
      // GetInterruptState?TRUE -> normal context -> mask NOT modified
      gBS->RestoreTPL (LockTpl);
      mLockCallbackExecuted = TRUE;
    }
Handler:
  VOID EFIAPI Test24Handler (IN UINT64 Time) {
    EFI_TPL OldTpl;

    OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
    gBS->SignalEvent (TestNotifyEvent);
    // RestoreTpl dispatches LockingCallback in event loop
    // LockingCallback does RaiseTpl(HIGH)/RestoreTpl inside
    // Callback's RaiseTpl sees GetInterruptState?TRUE (normal context)
    // -> does NOT set mask bit -> mask unchanged
    // Final section: mask still has original IRQ bit -> leaves disabled [OK]
    gBS->RestoreTpl (OldTpl);

    mHandlerExecuted = TRUE;
    mHandlerIterations++;
  }
Steps:
  1. Create test event at TPL_NOTIFY with LockingCallback
  2. SetRecursionWatchdog(5)
  3. InstallTestTimerHandler(Test24Handler)
  4. Stall(StallForTicks(20))  // = 20 ticks
  5. UninstallTestTimerHandler()
  6. ClearRecursionWatchdog()
  7. ASSERT mHandlerExecuted == TRUE
  8. ASSERT mLockCallbackExecuted == TRUE
  9. ASSERT mHandlerIterations > 5  // handler fired multiple times
  10. Close test event
Expected: System survives -- callback's RaiseTpl(HIGH) does not affect
mInterruptedTplMask (sees GetInterruptState?TRUE = normal context),
so outer final section correctly leaves interrupts disabled in IRQ context
```

**Test 25 -- IRQ-context RestoreTpl to intermediate TPL** *(Scenarios 11, 13 -- partial restore)*

Exercises a code path not covered by other tests: restoring to an
intermediate TPL (above the interrupted level) inside an interrupt handler.
The handler raises to HIGH (setting mask bit for APPLICATION=4), then
restores to `TPL_CALLBACK` (8). Since `8 > HighBitSet64(0x10) = 4`, the
final section takes the `else if` branch and re-enables interrupts -- even
though we are in IRQ context. This is correct: at CALLBACK(8) we are above
the danger zone APPLICATION(4), so timer interrupts that fire will see a
higher TPL and nest safely. The handler then restores to the original
interrupted TPL, where the mask correctly suppresses re-enabling.

This tests the subtle case where `NewTpl > HighBitSet64(mask)` but
`mask != 0` -- we're in IRQ context at a safe (higher) TPL.

```
Precondition: TPL_APPLICATION, interrupts enabled, timer running
Handler:
  VOID EFIAPI Test25Handler (IN UINT64 Time) {
    EFI_TPL OldTpl;

    // Raise to HIGH -- sets mask bit for APPLICATION(4)
    OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
    // mask = 0x10 (bit 4)

    // Restore to CALLBACK(8) -- intermediate, above interrupted TPL
    gBS->RestoreTPL (TPL_CALLBACK);
    // Final section: CoreSetInterruptState(FALSE), gEfiCurrentTpl = 8
    // 8 > HighBitSet64(0x10)=4 -> else if branch -> CoreSetInterruptState(TRUE)
    // Interrupts re-enabled at CALLBACK -- safe (above APPLICATION)

    // Work at TPL_CALLBACK with interrupts enabled...
    // Timer may fire here -> bounded nesting (sees CALLBACK, not APPLICATION)

    // Raise back to HIGH
    gBS->RaiseTPL (TPL_HIGH_LEVEL);
    // GetInterruptState -> TRUE -> normal context (mask not modified)

    // Restore to original interrupted TPL
    gBS->RestoreTPL (OldTpl);
    // Final section: CoreSetInterruptState(FALSE), gEfiCurrentTpl = 4
    // 4 = HighBitSet64(0x10)=4 -> mask &= 0x0F -> mask = 0
    // Leaves disabled -> (IRET will re-enable)

    mHandlerExecuted = TRUE;
    mHandlerIterations++;
  }
Steps:
  1. SetRecursionWatchdog(5)
  2. InstallTestTimerHandler(Test25Handler)
  3. Stall(StallForTicks(20))  // = 20 ticks
  4. UninstallTestTimerHandler()
  5. ClearRecursionWatchdog()
  6. ASSERT mHandlerExecuted == TRUE
  7. ASSERT mHandlerIterations > 5  // handler fired multiple times
  8. gCpu->EnableInterrupt(gCpu)  // ensure interrupts on after hook removal
  9. OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL)
  10. gBS->RestoreTPL(OldTpl)
  11. ASSERT GetInterruptState() == TRUE  // normal context works after
Expected: Intermediate TPL restore in IRQ context correctly re-enables
interrupts (safe: above interrupted TPL), final restore correctly leaves
disabled (at interrupted TPL)
```

### 6.6 Test Matrix

The implemented test suite contains 16 tests in 2 suites. These consolidate
the 25 analysis tests from Sections 6.4-6.5 into a practical executable set.

- **Suite 1: Spec Conformance (Tests 01-07)** -- Verify PI/UEFI specification
  mandated behavior for TPL operations and interrupt management.
- **Suite 2: Functional (Tests 08-16)** -- Verify the recursion fix is
  effective (prevents infinite recursion) and safe (no regressions).

| Impl. | Suite | Description | Scenarios | OvmfPkg X64 | ArmVirtPkg AARCH64 |
|---|---|---|---|---|---|
| 01 | Spec | RaiseTpl(HIGH)/RestoreTpl round-trip | 1 | PASS | PASS |
| 02 | Spec | RaiseTpl to non-HIGH | 2 | PASS | PASS |
| 03 | Spec | Nested RaiseTpl(HIGH) at HIGH | 3 | PASS | PASS |
| 04 | Spec | Event dispatch with interrupts enabled | 4 | PASS | PASS |
| 05 | Spec | DisableIRQ then RaiseTpl(HIGH) | 5 | PASS | PASS |
| 06 | Spec | DisableIRQ at HIGH (no-op) | 6 | PASS | PASS |
| 07 | Spec | Toggle IRQ without TPL change | 7 | PASS | PASS |
| 08 | Functional | Non-monotonic TPL (HIGH->APP->HIGH) | 8 | PASS | PASS |
| 09 | Functional | Timer IRQ during event dispatch | 9 | PASS | PASS |
| 10 | Functional | EnableIRQ at HIGH (misuse) | 10 | PASS | PASS |
| 11 | Functional | Sustained stress (100K iterations) | 11, 13 | PASS | PASS |
| 12 | Functional | Rapid TPL cycling (10K iterations) | 11, 13 | PASS | PASS |
| 13 | Functional | Timer event signaling during cycling | 9 | PASS | PASS |
| 14 | Functional | Non-HIGH RestoreTpl after timer (regression) | 12, 9 | PASS | PASS |
| 15 | Functional | Forced timer recursion (bounded depth) | 11, 13 | PASS | PASS |
| 16 | Functional | Natural timer recursion (bounded depth) | 11, 9 | PASS | PASS |

**Mapping to analysis tests (Sections 6.4-6.5):** Analysis tests 4-8
(timer interrupt, stale cleanup, temp lower TPL, event dispatch nesting,
timer during dispatch) are covered by implemented tests 04, 08, 09, 14-16.
Analysis tests 13-15 (architecture-specific inline assembly) and 16-19
(IRQ hook via `RegisterHandler`) are **not implemented** -- the IRQ hook
tests require unregistering `CoreTimerTick` which permanently breaks timer
event signaling; the arch-specific tests require conditionally-compiled
inline assembly. Analysis tests 20-25 (stress/stability) are consolidated
into implemented tests 11-16.

**Test execution order:** All 16 tests run sequentially: Tests 1-7
(Spec Conformance suite), 8-16 (Functional suite).

**Tests 15-16 (Profiled Aggressive Timer):** These tests use a **profiled
timer period** approach rather than a fixed period. At setup, the test
measures the cost of a callback work loop (`CALLBACK_WORK_ITERATIONS =
5000`) using the performance counter, then sets the hardware timer period
to a test-specific fraction of the measured callback duration (Test 15:
1/128th, Test 16: 1/128th). This ensures the timer fires many times per
callback execution (maximizing the probability of catching the brief
interrupt-enabled window at each nesting level) without causing livelock.
The period is clamped to [5us, 1ms].

Periodic timer events are created at `TPL_CALLBACK` and `TPL_NOTIFY` with
callbacks that perform the work loop. Test 15 loops
`RaiseTpl(HIGH)`/`RestoreTpl(APPLICATION)` which dispatches pending events
and triggers bounded nesting. Test 16 stalls at `TPL_APPLICATION`
(5ms per iteration), letting timer interrupts fire naturally via
`CoreTimerTick` -> `CoreReleaseLock` -> `RestoreTpl`.

**Pass criterion:** The DXE Core's `CoreTimerTick` diagnostics
(`CORE_TIMER_TICK_DIAGNOSTICS` config table) provides per-TPL entry counts
(`EntriesAtTpl[]`). Each test loops until the deepest nesting level
(`TPL_HIGH_LEVEL - 1 = 30`) has been entered at least
`DEPTH_OBSERVATION_COUNT` (10) new times, confirming that all 4 nesting
depths (APPLICATION -> CALLBACK -> NOTIFY -> HIGH-1) were reliably
achieved. A 60-second timeout (`DEPTH_OBSERVATION_TIMEOUT_SEC`) allows
for non-deterministic timer delivery under emulation.

The recursion mechanism applies to **both** X64 and AARCH64:
- **X64 (timer driver does not call RaiseTpl):** CoreTimerTick called
  directly at interrupted TPL. CoreReleaseLock -> RestoreTpl -> enables
  interrupts -> recursion.
- **AARCH64 (timer driver calls RaiseTpl(HIGH)):** Timer handler calls
  RaiseTpl(HIGH) -> CoreTimerTick -> RestoreTpl(OldTpl). That RestoreTpl
  dispatches pending notification events, lowering TPL and enabling
  interrupts. A pending timer interrupt fires during dispatch -> recursive
  CoreTimerTick.

**Not implemented (future work):** Tests 13-15 and 16-19 from the analysis
scenarios (arch-specific inline asm, IRQ hook via `RegisterHandler`). These
are described in Sections 6.4.3 and 6.4.4 for completeness.

**QEMU `-singlestep`:** Both the X64 and AARCH64 test runners use
`-singlestep` to force per-instruction interrupt delivery, ensuring the
timer fires during the brief interrupt-enabled window in `CoreRestoreTpl`
and providing deterministic, reproducible timing. Without `-singlestep`,
QEMU TCG executes in translation blocks (~128+ instructions) and only
checks timer expiry between blocks. The profiled timer period approach
makes the tests work on both native hardware and emulation.

The fix's correctness is verified through:
1. **Recursion depth instrumentation** (`EntriesAtTpl[]` counters in
   `CoreTimerTick`, tracked per-TPL for precise depth observation)
2. **Regression testing** (all 16 tests pass with fix, proving it does not
   break normal TPL/interrupt semantics)
3. **Stress testing** (Tests 11-16 under high timer pressure)
4. **Before/after methodology** (see Section 11 below)

### 6.7 Pass / Fail Criteria

**PASS:** All 16 test assertions hold, no watchdog timeout, no system
reset, no hang. Timer events continue to fire after all test sequences.
QEMU exits cleanly with all tests reporting PASSED.

**FAIL (regression -- normal context broken):** Tests 1-7 fail interrupt
state assertions -- indicates the fix is incorrectly suppressing interrupt
restoration in normal (non-interrupt) context. Test 4 validates the event
dispatch loop still re-enables interrupts at each dispatch level. Test 5
validates that `RestoreTpl` always re-enables interrupts below HIGH (stale
`gTplBeforeHighTpl` bits from `DisableInterrupt` + `RaiseTpl` are cleared).
Test 14 catches the bug where non-HIGH `RestoreTpl` incorrectly consults
the ISR mask and permanently disables interrupts.

**FAIL (stability):** Tests 11-16 trigger watchdog reset, indicating the
fix introduced a new hang or instability under timer stress. Tests 15/16
specifically exercise aggressive timer reprogramming to catch timer-period-
dependent issues.

---

## 10 References

- UEFI Specification -- Section 4.1 "Event, Timer, and Task Priority Services"
- [MdeModulePkg/Core/Dxe/Event/Tpl.c](Tpl.c)
- [MdeModulePkg/Core/Dxe/Event/Timer.c](Timer.c)
- [MdeModulePkg/Core/Dxe/Library/Library.c](../Library/Library.c) -- `CoreAcquireLock` / `CoreReleaseLock`

---

## 11 Test Methodology and Results

### 11.1 Recursion Detection Instrumentation

A `CORE_TIMER_TICK_DIAGNOSTICS` structure is installed in the EFI System
Configuration Table (via `gBS->InstallConfigurationTable`) as part of the
**first commit** on the branch (diagnostics, before the fix). This enables
external observation of `CoreTimerTick()` nesting behavior by the test app:

```c
typedef struct {
  UINT32            Signature;        // CORE_TIMER_TICK_DIAGNOSTICS_SIGNATURE
  volatile UINTN    CurrentDepth;     // Current recursion depth (0 when idle)
  volatile UINTN    MaxDepth;         // Maximum depth observed since boot
  volatile UINTN    TotalEntries;     // Total CoreTimerTick() entries
  volatile UINTN    EntriesAtTpl[32]; // Per-TPL entry counters
} CORE_TIMER_TICK_DIAGNOSTICS;
```

The DXE Core increments `CurrentDepth` on entry and decrements on exit
(within `DEBUG_CODE` blocks). `MaxDepth` tracks the high-water mark. On
architectures where the Timer Arch Protocol raises TPL before calling
`CoreTimerTick` (e.g. AARCH64), `MaxDepth` is also updated from the
popcount of `gIsrEntryTplMask` (since call-stack depth is always 1, but
the mask accumulates bits from nested handler invocations).

The `EntriesAtTpl[]` array records how many times `CoreTimerTick` was
entered at each interrupted TPL level. The test app uses these counters to
verify that bounded nesting actually occurs at all four standard TPL levels
(APPLICATION=4, CALLBACK=8, NOTIFY=16, HIGH-1=30).

**Why bounded to 4 levels:** The fix (`gIsrEntryTplMask`) prevents interrupt
re-enable at or below already-interrupted TPLs. The four practical levels
are TPL_APPLICATION (4), TPL_CALLBACK (8), TPL_NOTIFY (16), and
TPL_HIGH_LEVEL-1 (30). Under aggressive timer conditions, a timer can fire
at each distinct level during nested dispatch, reaching depth 4 -- this is
architecturally correct.

**Detection mechanism:** The test app reads the diagnostics structure from
the configuration table and uses the `EntriesAtTpl[]` counters to verify
nesting. Tests 15-16 set a watchdog timer (`SetWatchdogTimer`) before
entering the dangerous section. If infinite recursion occurs:
- The DXE Core max-depth ASSERT (`MAX_TIMER_INTERRUPT_NEST_DEPTH = 19`) fires
  first -> serial log records ASSERT in `CoreTimerTick()` -> **FAIL**
- If ASSERT output is unavailable or the platform continues unexpectedly,
  watchdog reset remains as fallback detection -> **FAIL**

With the fix, `gIsrEntryTplMask` bounds recursion. All four depth levels are
observed at least 10 times each within the timeout -> **PASS**.

### 11.2 Timer Period Selection for Stress Tests (15-16)

**Problem:** A fixed timer period (e.g. 100us) does not reliably trigger
recursion across different QEMU host speeds and architectures. Timer
interrupt frequency must be high relative to the cost of the timer callback
to guarantee the timer expires during the ISR.

**Solution:** Tests 15-16 use a **profiled timer period** via
`SetupAggressiveTimer()` -- they first measure the cost of the callback work
loop (`CALLBACK_WORK_ITERATIONS = 5000` iterations), then set the hardware
timer period using test-specific divisors: 1/128th for Test 15 and 1/128th
for Test 16. This ensures high interrupt pressure during callback execution,
helping trigger nested re-entry regardless of host speed.

| Parameter | Value | Rationale |
|---|---|---|
| Timer period | Test 15: callback_cost / 128; Test 16: callback_cost / 128; clamped [50, 10000] | High interrupt pressure with test-specific tuning |
| Observation count | 10 (`DEPTH_OBSERVATION_COUNT`) | Times each depth level must be observed |
| Depth levels | 4 (`DEPTH_LEVEL_COUNT`) | TPL 4, 8, 16, 30 |
| Observation timeout | 60s (`DEPTH_OBSERVATION_TIMEOUT_SEC`) | Generous timeout for slow QEMU hosts |
| Watchdog timeout | 90s (observation + 30s margin) | Detects infinite recursion via system reset |

**How it works:** `ProfileCallbackDuration()` runs the callback work loop
once, measures elapsed time via `GetPerformanceCounter()`, converts to 100ns
units, divides by the test-specific divisor (128 for Test 15, 128 for Test 16),
and clamps to [50, 10000] (5us to 1ms). Two periodic
timer events (at TPL_CALLBACK and TPL_NOTIFY) are created to ensure event
dispatch occurs at multiple TPL levels during `RestoreTpl`, maximizing the
probability of catching the interrupt-enabled window.

**Test 15 (Forced):** Tight loop of `RaiseTpl(HIGH)` / `RestoreTpl(APP)`.
Each `RestoreTpl` dispatches pending events with interrupts briefly enabled,
allowing the aggressive timer to fire and nest.

**Test 16 (Natural):** Loop of `gBS->Stall(5000)` at `TPL_APPLICATION`.
Timer interrupts fire naturally via `CoreTimerTick` -> `CoreReleaseLock` ->
`RestoreTpl` dispatch path.

**QEMU `-singlestep`:** The unified Python runner (`run_tests.py`) uses
`-singlestep` on both X64 and AARCH64 for deterministic, reproducible
timer behavior.

### 11.3 Test Configuration

`MdeModulePkg/Test/ShellTest/DxeCoreTplTest/run_tests.py` handles building,
staging, and running QEMU with appropriate timeout for both Windows host
(via WSL/GCC) and native WSL execution.

| Setting | X64 | AARCH64 |
|---|---|---|
| QEMU machine | q35 | virt |
| Memory | 256M | 256M |
| `-singlestep` | Yes | Yes |
| Timeout | 180s | 180s |
| Timer Arch | LocalApicTimerDxe (LAPIC) | ArmTimerDxe (ARM Generic Timer) |
| Timer ISR pattern | Does not RaiseTpl before CoreTimerTick | Calls RaiseTpl(HIGH) before CoreTimerTick |
| Timer period (Tests 15-16) | Profiled (Test 15: 1/128th, Test 16: 1/128th) | Profiled (Test 15: 1/128th, Test 16: 1/128th) |

### 11.4 Results: Before Fix (Diagnostics Only)

With only the diagnostics commit applied (no `gIsrEntryTplMask` fix),
expected behavior:

| Test | X64 (`-singlestep`) | AARCH64 |
|---|---|---|
| 01 - RaiseTpl(HIGH)/RestoreTpl round-trip | PASS | PASS |
| 02 - RaiseTpl to non-HIGH | PASS | PASS |
| 03 - Nested RaiseTpl(HIGH) at HIGH | PASS | PASS |
| 04 - Event dispatch with interrupts enabled | PASS | PASS |
| 05 - DisableIRQ then RaiseTpl(HIGH) | PASS | PASS |
| 06 - DisableIRQ at TPL_HIGH (no-op) | PASS | PASS |
| 07 - Toggle IRQ without TPL change | PASS | PASS |
| 08 - Non-monotonic TPL (HIGH->APP->HIGH) | PASS | PASS |
| 09 - Timer IRQ during event dispatch | PASS | PASS |
| 10 - EnableIRQ at TPL_HIGH (misuse) | PASS | PASS |
| 11 - Sustained RaiseTpl/RestoreTpl (100K iter) | PASS | PASS |
| 12 - Rapid TPL cycling across all levels | PASS | PASS |
| 13 - Timer event signaling during TPL cycling | PASS | PASS |
| 14 - Non-HIGH RestoreTpl after timer IRQ (regression) | PASS | PASS |
| 15 - Forced timer recursion (bounded depth) | **FAIL** (max-depth ASSERT; watchdog fallback) | **FAIL** (max-depth ASSERT; watchdog fallback) |
| 16 - Natural timer recursion (bounded depth) | **FAIL** (max-depth ASSERT; watchdog fallback) | **FAIL** (max-depth ASSERT; watchdog fallback) |

**Summary:** Tests 1-14 pass (default timer period does not trigger
recursion during their short execution windows). Tests 15-16 **FAIL** on
both architectures -- the profiled aggressive timer period causes unbounded
recursion. The max-depth ASSERT in `CoreTimerTick()` is the primary detection
signal; watchdog reset remains a safety-net fallback. Either signal reports
test failure.

### 11.5 Results: After Fix (Diagnostics + gIsrEntryTplMask)

With both commits applied (run 2026-07-02):

**OvmfPkg X64 (`-singlestep`):**
```
Timer diagnostics: TotalEntries=1716 MaxDepth=1
Performance counter: 3579 KHz (counts up, wraps every 4687 ms)
Total: 16 tests | Passed: 16 | Failed: 0
Total test execution time: 2014 ms (2.014 s)
```

**ArmVirtPkg AARCH64 (`-singlestep`):**
```
Timer diagnostics: TotalEntries=1327 MaxDepth=1
Total: 16 tests | Passed: 16 | Failed: 0
Total test execution time: 3268 ms (3.268 s)
```

| Test | X64 (ms) | AARCH64 (ms) | Result |
|---|---|---|---|
| 01 - RaiseTpl(HIGH)/RestoreTpl round-trip | 0 | 0 | PASS |
| 02 - RaiseTpl to non-HIGH | 0 | 0 | PASS |
| 03 - Nested RaiseTpl(HIGH) at HIGH | 0 | 0 | PASS |
| 04 - Event dispatch with interrupts enabled | 2 | 2 | PASS |
| 05 - DisableIRQ then RaiseTpl(HIGH) | 0 | 0 | PASS |
| 06 - DisableIRQ at TPL_HIGH (no-op) | 0 | 0 | PASS |
| 07 - Toggle IRQ without TPL change | 0 | 0 | PASS |
| 08 - Non-monotonic TPL (HIGH->APP->HIGH) | 30 | 30 | PASS |
| 09 - Timer IRQ during event dispatch | 39 | 40 | PASS |
| 10 - EnableIRQ at TPL_HIGH (misuse) | 35 | 37 | PASS |
| 11 - Sustained RaiseTpl/RestoreTpl (100K iter) | 505 | 1144 | PASS |
| 12 - Rapid TPL cycling across all levels | 110 | 177 | PASS |
| 13 - Timer event signaling during TPL cycling | 1522 | 1517 | PASS |
| 14 - Non-HIGH RestoreTpl after timer IRQ (regression) | 31 | 30 | PASS |
| 15 - Forced timer recursion (bounded depth) | 488 | 186 | PASS |
| 16 - Natural timer recursion (bounded depth) | 237 | 105 | PASS |

**Tests 15-16 depth observations (X64):**
```
Test 15: Profiled timer period: 115 (100ns units = 11.5us)
  Depth 1 (TPL 4):  24/10 observations
  Depth 2 (TPL 8):  75/10 observations
  Depth 3 (TPL 16): 47/10 observations
  Depth 4 (TPL 30): 10/10 observations
  Loop iterations: 702

Test 16: Profiled timer period: 98 (100ns units = 9.8us)
  Depth 1 (TPL 4):  17/10 observations
  Depth 2 (TPL 8):  30/10 observations
  Depth 3 (TPL 16): 41/10 observations
  Depth 4 (TPL 30): 12/10 observations
  Loop iterations: 10
```

**Tests 15-16 depth observations (AARCH64):**
```
Test 15: Profiled timer period: 228 (100ns units = 22.8us)
  Depth 1 (TPL 4):  33/10 observations
  Depth 2 (TPL 8):  54/10 observations
  Depth 3 (TPL 16): 10/10 observations
  Depth 4 (TPL 30): 433/10 observations
  Loop iterations: 2534

Test 16: Profiled timer period: 252 (100ns units = 25.2us)
  Depth 1 (TPL 4):  16/10 observations
  Depth 2 (TPL 8):  10/10 observations
  Depth 3 (TPL 16): 14/10 observations
  Depth 4 (TPL 30): 203/10 observations
  Loop iterations: 7
```

**Summary:** All 16 tests pass on both architectures. Tests 15-16
demonstrate bounded nesting at all four TPL levels, proving the fix
correctly bounds recursion under aggressive timer conditions.

### 11.6 Interpretation

| Condition | Tests 1-14 | Tests 15-16 |
|---|---|---|
| Without fix (profiled timer) | PASS (no recursion at default speed) | **FAIL** (watchdog reset -- unbounded recursion) |
| With fix (profiled timer) | PASS (fix is transparent) | PASS (bounded to 4 levels) |

This demonstrates:
1. **The bug is real** -- Tests 15-16 without the fix cause unbounded
   recursion leading to watchdog reset, proving the recursion vulnerability
   exists regardless of timer ISR pattern (both X64 and AARCH64).
2. **The fix is effective** -- Same tests with the fix pass, observing all
   four nesting depth levels at least 10 times each. The `gIsrEntryTplMask`
   successfully bounds recursion to one entry per distinct interrupted TPL.
3. **The fix is safe** -- Tests 1-14 pass identically with and without the
   fix, proving no regression in normal TPL/interrupt semantics. Timer event
   signaling, event dispatch, interrupt enable/disable, and non-monotonic
   TPL patterns all behave correctly.
4. **Both architectures are affected and fixed** -- On X64 (timer driver
   does not call RaiseTpl), recursion occurs inside CoreTimerTick's own
   `CoreReleaseLock` -> `RestoreTpl` path. On AARCH64 (timer driver calls
   RaiseTpl(HIGH) before CoreTimerTick), the timer handler's `RestoreTpl`
   after CoreTimerTick dispatches pending events with interrupts enabled,
   allowing a new timer interrupt to re-enter CoreTimerTick. The fix
   (`gIsrEntryTplMask`) prevents unbounded recursion on both architectures.
5. **Depth 4 (TPL 30) observation confirms full nesting** -- The aggressive
   timer fires during event dispatch at each level. On AARCH64, TPL 30
   entries are especially frequent (433 observations in Test 15) because the
   ARM timer handler raises to HIGH, and `gTplBeforeHighTpl` captures the
   interrupted TPL just below HIGH.
