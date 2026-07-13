# DXE Core TPL Timer Interrupt Recursion Test

## Overview

This UEFI Shell application verifies that the `gIsrEntryTplMask`-based logic
in DXE Core TPL handling prevents infinite
recursion from timer interrupts while preserving normal TPL and interrupt
behavior per the PI and UEFI specifications.

## Building

### Standalone (test app only)

```
build -p MdeModulePkg/Test/ShellTest/DxeCoreTplTest/DxeCoreTplTest.dsc -a X64 -t VS2022 -b DEBUG
```

Output: `Build/DxeCoreTplTest/DEBUG_VS2022/X64/DxeCoreTplTestApp.efi`

### As part of OVMF (includes the DXE Core fix)

```
build -p OvmfPkg/OvmfPkgIa32X64.dsc -a IA32 -a X64 -t VS2022 -b DEBUG -D NETWORK_ENABLE=FALSE
```

## Running in QEMU

### Automated runners (recommended)

Use the unified Python runner:
`MdeModulePkg/Test/ShellTest/DxeCoreTplTest/run_tests.py`.

Prerequisites:
- QEMU installed and available.
- On Windows, runner lookup order for QEMU binaries is:
  1) `--qemu-x64` / `--qemu-aarch64` option (if provided)
  2) default paths:
  - `C:\Program Files\qemu\qemu-system-x86_64.exe`
  - `C:\Program Files\qemu\qemu-system-aarch64.exe`
  3) executable found in `PATH`
- On WSL/Linux, runner lookup order is:
  1) `--qemu-x64` / `--qemu-aarch64` option (if provided)
  2) executable found in `PATH`

#### Windows host (native build and run)

From repository root:

```
python .\MdeModulePkg\Test\ShellTest\DxeCoreTplTest\run_tests.py
```

The Windows runner path builds and runs natively on Windows and adds
`-D NETWORK_ENABLE=FALSE`.

#### Native WSL/Linux

From repository root (inside WSL):

```
python3 ./MdeModulePkg/Test/ShellTest/DxeCoreTplTest/run_tests.py
```

The native WSL/Linux path builds with `-D NETWORK_ENABLE=FALSE` for both
platforms.

#### Common runner options

The same options are available on both Windows and native WSL/Linux:

```
--platform X64
--platform AARCH64
--build-only
--run-only
--clean
--verbose
--timeout-seconds 300
--toolchain <tag>
--qemu-x64 <path-or-command>
--qemu-aarch64 <path-or-command>
```

Toolchain defaults:
- Windows default: `CLANGPDB` (supports both X64 and AARCH64)
- WSL/Linux default: `GCC`

Examples:

```
python .\MdeModulePkg\Test\ShellTest\DxeCoreTplTest\run_tests.py --platform X64 --clean
python3 ./MdeModulePkg/Test/ShellTest/DxeCoreTplTest/run_tests.py --platform AARCH64 --timeout-seconds 300
python .\MdeModulePkg\Test\ShellTest\DxeCoreTplTest\run_tests.py --toolchain CLANGPDB --platform Both
python .\MdeModulePkg\Test\ShellTest\DxeCoreTplTest\run_tests.py --qemu-x64 D:\Tools\qemu-system-x86_64.exe --qemu-aarch64 D:\Tools\qemu-system-aarch64.exe
python3 ./MdeModulePkg/Test/ShellTest/DxeCoreTplTest/run_tests.py --qemu-x64 /usr/local/bin/qemu-system-x86_64 --qemu-aarch64 /usr/local/bin/qemu-system-aarch64
```

### Manual QEMU setup

1. Copy the test EFI and Shell binary to a FAT test drive:

```
mkdir Build\OvmfTest\EFI\BOOT
copy Build\DxeCoreTplTest\DEBUG_GCC\X64\DxeCoreTplTestApp.efi Build\OvmfTest\
copy Build\OvmfX64\DEBUG_GCC\X64\Shell.efi Build\OvmfTest\EFI\BOOT\BOOTX64.EFI
copy Build\OvmfX64\DEBUG_GCC\FV\OVMF_CODE.fd Build\OVMF_CODE.fd
copy Build\OvmfX64\DEBUG_GCC\FV\OVMF_VARS.fd Build\OVMF_VARS.fd
```

2. Create `Build\OvmfTest\startup.nsh`:

```
map -r
fs0:
DxeCoreTplTestApp.efi
reset -s
```

3. Launch QEMU:

```
qemu-system-x86_64 ^
  -machine q35 -m 256 -singlestep ^
  -drive if=pflash,format=raw,readonly=on,file=Build\OVMF_CODE.fd ^
  -drive if=pflash,format=raw,file=Build\OVMF_VARS.fd ^
  -drive file=fat:rw:Build\OvmfTest,format=raw ^
  -nic none -display none -serial stdio -monitor none
```

Test results print to serial (stdout). The VM shuts down automatically after tests complete.

## Test Suites and Cases

### Suite 1: PI/UEFI Spec Conformance (Tests 1-7)

These tests verify PI/UEFI specification mandated behavior for TPL operations
and interrupt management.

| Test | Description |
|------|-------------|
| 01 | RaiseTpl(HIGH)/RestoreTpl — interrupts disabled at HIGH, re-enabled on restore |
| 02 | RaiseTpl to non-HIGH (NOTIFY) — interrupts remain enabled |
| 03 | Nested RaiseTpl(HIGH) at HIGH — returns HIGH, idempotent |
| 04 | Event dispatch with interrupts enabled during RestoreTpl |
| 05 | DisableInterrupt then RaiseTpl(HIGH) — RestoreTpl re-enables |
| 06 | DisableInterrupt at HIGH — no-op, RestoreTpl re-enables |
| 07 | Toggle interrupts without TPL change — no TPL side effects |

### Suite 2: Recursion Fix Functional Tests (Tests 8-16)

These tests verify the recursion fix is effective (prevents infinite recursion)
and safe (no regressions under load). Infinite-recursion regression detection
for aggressive timer tests is assert-first (max nest-depth ASSERT), with
watchdog reset as a fallback safety net.

| Test | Description |
|------|-------------|
| 08 | Non-monotonic TPL transitions (HIGH -> APP -> HIGH) |
| 09 | Timer IRQ during event dispatch — no crash, self-cleans |
| 10 | EnableInterrupt at HIGH (misuse) — fix prevents recursion |
| 11 | Sustained RaiseTpl/RestoreTpl — 100,000 iterations |
| 12 | Rapid TPL cycling across all levels — 10,000 iterations |
| 13 | Timer event signaling during TPL cycling — fix doesn't suppress |
| 14 | Non-HIGH RestoreTpl after timer IRQ — regression guard |
| 15 | Forced timer recursion — bounded depth (profiled aggressive timer, divisor 1/128) |
| 16 | Natural timer recursion — bounded depth (stall-based, divisor 1/128) |

## Timing

Each test case reports its execution time in milliseconds using the platform's
performance counter (via `GetPerformanceCounter()` / `GetPerformanceCounterProperties()`
from TimerLib).  On platforms with narrow counters (e.g. the 24-bit ACPI PM
Timer on X64 which wraps every ~4.7 seconds), tests that exceed the wrap
period automatically fall back to `gRT->GetTime()` wall-clock timing (second
resolution).  On platforms with wide counters (e.g. the 64-bit ARM Generic
Timer at 62.5 MHz), millisecond-precision performance counter timing is used
for all tests.

`run_tests.py` runs QEMU with `-singlestep` on both X64 and AARCH64 for
deterministic, reproducible timing.

## Expected Output

```
DxeCoreTplTestApp: Starting...
Performance counter: 3579 KHz (counts up, wraps every 4687 ms)
  [Timing] Test 01: 2 ms
  [Timing] Test 02: 0 ms
  ...
  [Timing] Test 15: <N> ms
  [Timing] Test 16: <N> ms
---------------------------------------------------------
------------- UNIT TEST FRAMEWORK RESULTS ---------------
---------------------------------------------------------
  ...
=========================================================
  TEST SUMMARY
---------------------------------------------------------
  Test  | Result | Time (ms)
  ------+--------+----------
    01  | PASS |  2
    ...
    16  | PASS |  <N>
---------------------------------------------------------
  Total: 16 tests | Passed: 16 | Failed: 0
  Total test execution time: <N> ms (<N>.<NNN> s)
=========================================================
```

All 16 tests should pass.

If failures occur, treat them as a signal to inspect logs first: they may
indicate recursion-regression behavior, but can also be caused by environment
issues (for example QEMU launch errors or timeout conditions).
