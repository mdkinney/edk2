# DXE Core TPL Timer Interrupt Recursion Test

## Overview

This UEFI Shell application verifies that the `mIsrEntryTplMask`-based fix
in `CoreRaiseTpl`/`CoreRestoreTpl` (MdeModulePkg/Core/Dxe/Event/Tpl.c) prevents
infinite recursion from timer interrupts while preserving normal TPL and
interrupt behavior.

## Building

### Standalone (test app only)

```
build -p MdeModulePkg/Test/ShellTest/DxeCoreTplTest/DxeCoreTplTest.dsc -a X64 -t VS2022 -b DEBUG
```

Output: `Build/DxeCoreTplTest/DEBUG_VS2022/X64/DxeCoreTplTestApp.efi`

### As part of OVMF (includes the DXE Core fix)

```
build -p OvmfPkg/OvmfPkgIa32X64.dsc -a IA32 -a X64 -t VS2022 -b DEBUG
```

## Running in QEMU

1. Copy the test EFI to the OVMF virtual drive:

```
mkdir Build\Ovmf3264\DEBUG_VS2022\VirtualDrive
copy Build\DxeCoreTplTest\DEBUG_VS2022\X64\DxeCoreTplTestApp.efi Build\Ovmf3264\DEBUG_VS2022\VirtualDrive\
```

2. Create `Build\Ovmf3264\DEBUG_VS2022\VirtualDrive\startup.nsh`:

```
DxeCoreTplTestApp.efi
reset -s
```

3. Launch QEMU:

```
qemu-system-x86_64 ^
  -machine q35,smm=on -m 256 -smp 1 ^
  -pflash Build/Ovmf3264/DEBUG_VS2022/FV/OVMF.fd ^
  -drive file=fat:rw:Build/Ovmf3264/DEBUG_VS2022/VirtualDrive ^
  -serial stdio -display none -no-reboot ^
  -debugcon file:Build/Ovmf3264/DEBUG_VS2022/debug.log ^
  -global isa-debugcon.iobase=0x402
```

Test results print to serial (stdout). The VM shuts down automatically after tests complete.

## Test Suites and Cases

### Suite 1: TPL-Managed Interrupt Scenarios (Tests 1-8, 26)

| Test | Description |
|------|-------------|
| 01 | Normal RaiseTpl(HIGH)/RestoreTpl — basic TPL round-trip |
| 02 | RaiseTpl to non-HIGH levels (CALLBACK, NOTIFY) |
| 03 | Nested RaiseTpl(HIGH) when already at HIGH |
| 04 | No recursion under sustained timer load (1000 iterations) |
| 05 | Normal context restored after timer IRQ |
| 06 | Temporarily lower TPL across HIGH boundary |
| 07 | Normal TPL nesting via event dispatch |
| 08 | Timer IRQ fires during normal event dispatch |
| 26 | TPL preemption hierarchy: CALLBACK preempted by NOTIFY via timer |

### Suite 2: CPU Protocol Interrupt Manipulation (Tests 9-12)

| Test | Description |
|------|-------------|
| 09 | DisableInterrupt then RaiseTpl(HIGH) |
| 10 | DisableInterrupt while at TPL_HIGH |
| 11 | Toggle interrupts without TPL change |
| 12 | EnableInterrupt at TPL_HIGH (misuse scenario) |

### Suite 3: Architecture-Specific Interrupt Instructions (Tests 13-15)

| Test | Description |
|------|-------------|
| 13 | CLI then RaiseTpl(HIGH) |
| 14 | CLI/STI bracket without TPL change |
| 15 | STI at TPL_HIGH (misuse scenario) |

### Suite 4: Stress and Stability (Tests 20-23)

| Test | Description |
|------|-------------|
| 20 | Sustained RaiseTpl/RestoreTpl — 100,000 iterations |
| 21 | Rapid TPL cycling across all levels — 10,000 iterations |
| 22 | Timer event signaling during TPL cycling |
| 23 | Non-HIGH RestoreTpl after timer IRQ (regression test) |

### Suite 5: IRQ-Context Tests via Timer Handler Hook (Tests 16-19, 24-25)

These tests replace `CoreTimerTick` with a custom handler and execute test
logic directly in interrupt context. They run last because they permanently
unregister the DXE Core timer handler.

| Test | Description |
|------|-------------|
| 16 | IRQ-context RaiseTpl/RestoreTpl |
| 17 | IRQ-context event dispatch |
| 18 | IRQ-context sustained stress |
| 19 | IRQ-context temporarily lower TPL |
| 24 | IRQ-context callback with RaiseTpl(HIGH) |
| 25 | IRQ-context intermediate TPL restore |

## Timing

Each test case reports its execution time in milliseconds using the platform's
performance counter (via `GetPerformanceCounter()` / `GetPerformanceCounterProperties()`
from TimerLib). This is portable across IA32, X64, and AARCH64. Total execution
time is printed after all suites complete.

## Expected Output

```
DxeCoreTplTestApp: Starting...
Performance counter: <N> KHz (counts up)
  [Timing] Test 01: 0 ms
  [Timing] Test 02: 0 ms
  ...
  [Timing] Test 25: <N> ms
  [Timing] Test 26: <N> ms
---------------------------------------------------------
------------- UNIT TEST FRAMEWORK RESULTS ---------------
---------------------------------------------------------
  ...
=========================================================
Total Stats
 Passed:  26  (100%)
 Failed:  0  (0%)
 Not Run: 0  (0%)
=========================================================
  Total test execution time: <N> ms (<N>.<NNN> s)
=========================================================
```

All 26 tests should pass. Any failure indicates a regression in the
`mIsrEntryTplMask` timer interrupt recursion prevention logic.
