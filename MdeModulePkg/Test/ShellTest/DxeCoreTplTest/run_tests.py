#!/usr/bin/env python3
"""Unified DxeCoreTplTest runner used by both Windows and WSL wrappers."""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path
from typing import Iterable, List, Sequence


EXPECTED_TEST_COUNT = 16
EXCEPTION_PATTERNS = (
    r"Synchronous Exception",
    r"Recursive exception occurred",
    r"Exception Type",
    r"#PF - Page-Fault",
)

MAX_DEPTH_ASSERT_PATTERNS = (
    r"ASSERT.*Timer\.c.*MAX_TIMER_INTERRUPT_NEST_DEPTH",
    r"ASSERT.*CurrentDepth\s*<=\s*MAX_TIMER_INTERRUPT_NEST_DEPTH",
    r"ASSERT.*CurrentDepth\s*<=\s*19",
    r"ASSERT.*Depth\s*<=\s*MAX_TIMER_INTERRUPT_NEST_DEPTH",
    r"ASSERT.*Depth\s*<=\s*\d+",
    r"ASSERT.*Tpl\.c.*MAX_INTERRUPT_ENABLE_NEST_DEPTH",
    r"ASSERT.*mInterruptEnableNestDepth\s*<\s*MAX_INTERRUPT_ENABLE_NEST_DEPTH",
    (
        r"ASSERT\s*\[DxeCore\]\s*Tpl\.c\(\d+\):\s*"
        r"mInterruptEnableNestDepth\s*<\s*\(\s*3\s*\+\s*16\s*\)"
    ),
    (
        r"ASSERT.*gIsrEntryTplMask\s*&\s*\(1ULL\s*<<\s*InterruptedTpl\)\)"
        r"\s*==\s*0"
    ),
    (
        r"ASSERT.*gIsrEntryTplMask\s*&\s*\(1ULL\s*<<\s*gEfiCurrentTpl\)\)"
        r"\s*==\s*0"
    ),
)

EARLY_ASSERT_ABORT_RC = 125


def is_windows_host() -> bool:
    return os.name == "nt"


def resolve_qemu_executable(
    arch: str,
    *,
    windows_override: str | None = None,
    posix_override: str | None = None,
) -> str:
    if arch not in ("X64", "AARCH64"):
        raise ValueError(f"Unsupported arch for QEMU resolution: {arch}")

    if is_windows_host():
        if windows_override:
            if Path(windows_override).exists():
                return windows_override
            raise FileNotFoundError(f"QEMU executable not found: {windows_override}")

        default = (
            r"C:\Program Files\qemu\qemu-system-x86_64.exe"
            if arch == "X64"
            else r"C:\Program Files\qemu\qemu-system-aarch64.exe"
        )
        if Path(default).exists():
            return default

        fallback_name = "qemu-system-x86_64.exe" if arch == "X64" else "qemu-system-aarch64.exe"
        found = shutil.which(fallback_name)
        if found:
            return found

        raise FileNotFoundError(
            f"QEMU executable for {arch} not found. Tried '{default}' and PATH lookup for '{fallback_name}'."
        )

    if posix_override:
        if shutil.which(posix_override) or Path(posix_override).exists():
            return posix_override
        raise FileNotFoundError(f"QEMU executable not found: {posix_override}")

    fallback_name = "qemu-system-x86_64" if arch == "X64" else "qemu-system-aarch64"
    found = shutil.which(fallback_name)
    if found:
        return found

    raise FileNotFoundError(
        f"QEMU executable for {arch} not found in PATH ('{fallback_name}')."
    )


def has_max_depth_assert(text: str) -> bool:
    return any(
        re.search(pattern, text)
        for pattern in MAX_DEPTH_ASSERT_PATTERNS
    )


def print_cmd(cmd: Sequence[str], verbose: bool) -> None:
    if verbose:
        joined = " ".join(cmd)
        print(f"    [CMD] {joined}")


def run_cmd(
    cmd: Sequence[str],
    *,
    cwd: Path | None = None,
    verbose: bool = False,
    check: bool = True,
    capture: bool = False,
) -> subprocess.CompletedProcess[str]:
    print_cmd(cmd, verbose)
    return subprocess.run(
        cmd,
        cwd=str(cwd) if cwd else None,
        check=check,
        text=True,
        capture_output=capture,
    )


def build_firmware_windows(
    edk2_root: Path,
    arch: str,
    dsc: str,
    *,
    toolchain: str,
    clean: bool,
    verbose: bool,
) -> None:
    print(f"  Building {arch} firmware...")
    if arch == "AARCH64":
        if clean:
            shutil.rmtree(edk2_root / "Build" / "ArmVirtQemu-AArch64", ignore_errors=True)
    elif clean:
        shutil.rmtree(edk2_root / "Build" / "OvmfX64", ignore_errors=True)

    build_cmd = (
        f"build -a {arch} -t {toolchain} -p {dsc} -b DEBUG "
        f"-D NETWORK_ENABLE=FALSE "
        f"--pcd gEfiShellPkgTokenSpaceGuid.PcdShellDefaultDelay=0 "
        f"--pcd gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut=0"
    )
    if arch == "AARCH64":
        # Enable DEBUG print/code and keep deadloop-only ASSERT (no breakpoint detour).
        build_cmd += " --pcd gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask=0x27"
    if arch == "X64":
        build_cmd += " -D DEBUG_ON_SERIAL_PORT"
    run_cmd(["cmd", "/c", f"call edksetup.bat && {build_cmd}"], cwd=edk2_root, verbose=verbose)


def build_test_app_windows(
    edk2_root: Path,
    arch: str,
    *,
    toolchain: str,
    clean: bool,
    verbose: bool,
) -> None:
    print(f"  Building {arch} test app...")
    if clean:
        shutil.rmtree(edk2_root / "Build" / "DxeCoreTplTest" / f"DEBUG_{toolchain}" / arch, ignore_errors=True)
    build_cmd = "build -a {} -t {} -p MdeModulePkg/Test/ShellTest/DxeCoreTplTest/DxeCoreTplTest.dsc -b DEBUG".format(arch, toolchain)
    if arch == "AARCH64":
        build_cmd += " --pcd gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask=0x27"
    run_cmd(["cmd", "/c", f"call edksetup.bat && {build_cmd}"], cwd=edk2_root, verbose=verbose)


def build_firmware_native(edk2_root: Path, arch: str, dsc: str, *, toolchain: str, clean: bool, verbose: bool) -> None:
    print(f"  Building {arch} firmware...")
    parts = [f"cd {edk2_root}", "source edksetup.sh >/dev/null 2>&1"]
    if arch == "AARCH64":
        parts.append("export GCC_AARCH64_PREFIX=${GCC_AARCH64_PREFIX:-aarch64-linux-gnu-}")
        if clean:
            parts.append("rm -rf Build/ArmVirtQemu-AArch64")
    elif clean:
        parts.append("rm -rf Build/OvmfX64")
    build_cmd = (
        f"build -a {arch} -t {toolchain} -p {dsc} -b DEBUG "
        f"-D NETWORK_ENABLE=FALSE "
        f"--pcd gEfiShellPkgTokenSpaceGuid.PcdShellDefaultDelay=0 "
        f"--pcd gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut=0"
    )
    if arch == "AARCH64":
        # Enable DEBUG print/code and keep deadloop-only ASSERT (no breakpoint detour).
        build_cmd += " --pcd gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask=0x27"
    if arch == "X64":
        build_cmd += " -D DEBUG_ON_SERIAL_PORT"
    if not verbose:
        build_cmd += " 2>&1 | tail -5"
    parts.append(build_cmd)
    run_cmd(["bash", "-lc", " && ".join(parts)], verbose=verbose)


def build_test_app_native(edk2_root: Path, arch: str, *, toolchain: str, clean: bool, verbose: bool) -> None:
    print(f"  Building {arch} test app...")
    parts = [f"cd {edk2_root}", "source edksetup.sh >/dev/null 2>&1"]
    if arch == "AARCH64":
        parts.append("export GCC_AARCH64_PREFIX=${GCC_AARCH64_PREFIX:-aarch64-linux-gnu-}")
    if clean:
        parts.append(f"rm -rf Build/DxeCoreTplTest/DEBUG_{toolchain}/{arch}")
    build_cmd = f"build -a {arch} -t {toolchain} -p MdeModulePkg/Test/ShellTest/DxeCoreTplTest/DxeCoreTplTest.dsc -b DEBUG"
    if arch == "AARCH64":
        build_cmd += " --pcd gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask=0x27"
    if not verbose:
        build_cmd += " 2>&1 | tail -5"
    parts.append(build_cmd)
    run_cmd(["bash", "-lc", " && ".join(parts)], verbose=verbose)


def stage_aarch64_windows(
    win_edk2: Path,
    toolchain: str,
    verbose: bool,
) -> None:
    print("  Staging AARCH64 artifacts...")
    build_root = win_edk2 / "Build"
    shutil.copy2(win_edk2 / f"Build/ArmVirtQemu-AArch64/DEBUG_{toolchain}/FV/QEMU_EFI.fd", build_root / "QEMU_EFI.fd")
    with open(build_root / "QEMU_EFI.fd", "r+b") as fw:
        fw.truncate(64 * 1024 * 1024)
    stage_dir = build_root / "ArmVirtTest"
    if stage_dir.exists():
        shutil.rmtree(stage_dir)
    (stage_dir / "EFI" / "BOOT").mkdir(parents=True, exist_ok=True)
    shutil.copy2(win_edk2 / f"Build/DxeCoreTplTest/DEBUG_{toolchain}/AARCH64/DxeCoreTplTestApp.efi", stage_dir / "DxeCoreTplTestApp.efi")
    shutil.copy2(win_edk2 / f"Build/ArmVirtQemu-AArch64/DEBUG_{toolchain}/AARCH64/Shell.efi", stage_dir / "EFI/BOOT/BOOTAA64.EFI")
    (stage_dir / "startup.nsh").write_text(
        "map -r\n"
        "fs0:\n"
        "DxeCoreTplTestApp.efi\n"
        "reset -s",
        encoding="ascii",
    )


def stage_x64_windows(
    win_edk2: Path,
    toolchain: str,
    verbose: bool,
) -> None:
    print("  Staging X64 artifacts...")
    build_root = win_edk2 / "Build"
    shutil.copy2(win_edk2 / f"Build/OvmfX64/DEBUG_{toolchain}/FV/OVMF_CODE.fd", build_root / "OVMF_CODE.fd")
    shutil.copy2(win_edk2 / f"Build/OvmfX64/DEBUG_{toolchain}/FV/OVMF_VARS.fd", build_root / "OVMF_VARS.fd")
    stage_dir = build_root / "OvmfTest"
    if stage_dir.exists():
        shutil.rmtree(stage_dir)
    (stage_dir / "EFI" / "BOOT").mkdir(parents=True, exist_ok=True)
    shutil.copy2(win_edk2 / f"Build/DxeCoreTplTest/DEBUG_{toolchain}/X64/DxeCoreTplTestApp.efi", stage_dir / "DxeCoreTplTestApp.efi")
    shutil.copy2(win_edk2 / f"Build/OvmfX64/DEBUG_{toolchain}/X64/Shell.efi", stage_dir / "EFI/BOOT/BOOTX64.EFI")
    (stage_dir / "startup.nsh").write_text(
        "map -r\n"
        "fs0:\n"
        "DxeCoreTplTestApp.efi\n"
        "reset -s",
        encoding="ascii",
    )


def stage_aarch64_native(edk2_root: Path, toolchain: str) -> None:
    print("  Staging AARCH64 artifacts...")
    build_root = edk2_root / "Build"
    shutil.copy2(edk2_root / f"Build/ArmVirtQemu-AArch64/DEBUG_{toolchain}/FV/QEMU_EFI.fd", build_root / "QEMU_EFI.fd")
    run_cmd(["truncate", "-s", "64M", str(build_root / "QEMU_EFI.fd")])
    stage_dir = build_root / "ArmVirtTest"
    if stage_dir.exists():
        shutil.rmtree(stage_dir)
    (stage_dir / "EFI" / "BOOT").mkdir(parents=True, exist_ok=True)
    shutil.copy2(edk2_root / f"Build/DxeCoreTplTest/DEBUG_{toolchain}/AARCH64/DxeCoreTplTestApp.efi", stage_dir / "DxeCoreTplTestApp.efi")
    shutil.copy2(edk2_root / f"Build/ArmVirtQemu-AArch64/DEBUG_{toolchain}/AARCH64/Shell.efi", stage_dir / "EFI/BOOT/BOOTAA64.EFI")
    (stage_dir / "startup.nsh").write_text(
        "map -r\n"
        "fs0:\n"
        "mkdir UTCache\n"
        "echo cache_write_test > UTCache\\probe.txt\n"
        "ls UTCache\n"
        "DxeCoreTplTestApp.efi --CachePath UTCache\n"
        "reset -s",
        encoding="ascii",
    )


def stage_x64_native(edk2_root: Path, toolchain: str) -> None:
    print("  Staging X64 artifacts...")
    build_root = edk2_root / "Build"
    shutil.copy2(edk2_root / f"Build/OvmfX64/DEBUG_{toolchain}/FV/OVMF_CODE.fd", build_root / "OVMF_CODE.fd")
    shutil.copy2(edk2_root / f"Build/OvmfX64/DEBUG_{toolchain}/FV/OVMF_VARS.fd", build_root / "OVMF_VARS.fd")
    stage_dir = build_root / "OvmfTest"
    if stage_dir.exists():
        shutil.rmtree(stage_dir)
    (stage_dir / "EFI" / "BOOT").mkdir(parents=True, exist_ok=True)
    shutil.copy2(edk2_root / f"Build/DxeCoreTplTest/DEBUG_{toolchain}/X64/DxeCoreTplTestApp.efi", stage_dir / "DxeCoreTplTestApp.efi")
    shutil.copy2(edk2_root / f"Build/OvmfX64/DEBUG_{toolchain}/X64/Shell.efi", stage_dir / "EFI/BOOT/BOOTX64.EFI")
    (stage_dir / "startup.nsh").write_text(
        "map -r\n"
        "fs0:\n"
        "mkdir UTCache\n"
        "echo cache_write_test > UTCache\\probe.txt\n"
        "ls UTCache\n"
        "DxeCoreTplTestApp.efi --CachePath UTCache\n"
        "reset -s",
        encoding="ascii",
    )


def stream_serial_updates(output_file: Path, arch: str, offset: int) -> int:
    if not output_file.exists():
        return offset
    with output_file.open("r", encoding="utf-8", errors="replace") as fh:
        fh.seek(offset)
        data = fh.read()
        new_offset = fh.tell()
    if data:
        for line in data.splitlines():
            print(f"    [{arch}] {line}")
    return new_offset


def run_qemu(
    exe: str,
    args: Sequence[str],
    *,
    cwd: Path,
    output_file: Path,
    arch: str,
    timeout_seconds: int,
    verbose: bool,
) -> tuple[int, float]:
    if output_file.exists():
        output_file.unlink()
    if verbose:
        output_file.touch()
        print_cmd([exe, *args], True)

    proc = subprocess.Popen([exe, *args], cwd=str(cwd))
    start = time.monotonic()
    timeout_start = start
    timeout_window_reset = False
    offset = 0
    recent_serial_tail = ""

    while True:
        if output_file.exists():
            with output_file.open("r", encoding="utf-8", errors="replace") as fh:
                fh.seek(offset)
                data = fh.read()
                offset = fh.tell()
            if data:
                recent_serial_tail = (recent_serial_tail + data)[-2048:]
                if verbose:
                    for line in data.splitlines():
                        print(f"    [{arch}] {line}")

                if has_max_depth_assert(recent_serial_tail):
                    proc.kill()
                    proc.wait(timeout=5)
                    time.sleep(0.2)
                    elapsed = time.monotonic() - start
                    print(
                        "  Detected max interrupt nest-depth ASSERT in serial output; "
                        "stopping QEMU early"
                    )
                    return EARLY_ASSERT_ABORT_RC, elapsed

                # On slow hosts with -singlestep, firmware DEBUG boot can dominate wall time.
                # Reset the timeout once the actual test app starts so the timeout budget
                # tracks test execution rather than pre-test boot overhead.
                if (not timeout_window_reset) and ("DxeCoreTplTestApp: Starting..." in recent_serial_tail):
                    timeout_start = time.monotonic()
                    timeout_window_reset = True
                    print("  Detected test app start; resetting timeout window")
        rc = proc.poll()
        if rc is not None:
            if verbose:
                stream_serial_updates(output_file, arch, offset)
            elapsed = time.monotonic() - start
            print(f"  QEMU exited in {elapsed:.1f}s (suggested timeout: {int((elapsed * 1.25) + 0.999)}s)")
            return rc, elapsed
        if time.monotonic() - timeout_start >= timeout_seconds:
            proc.kill()
            proc.wait(timeout=5)
            time.sleep(0.5)
            elapsed = time.monotonic() - start
            print(f"  QEMU killed after {elapsed:.1f}s due to timeout")
            return 124, elapsed
        time.sleep(0.2)


def parse_results(output_file: Path, arch: str, qemu_rc: int, host_elapsed_seconds: float) -> bool:
    if not output_file.exists():
        print("  ERROR: No output file found!")
        return False
    if output_file.stat().st_size == 0:
        print("  ERROR: Output file is empty (firmware likely crashed or timed out)!")
        return False

    lines = output_file.read_text(encoding="utf-8", errors="replace").splitlines()
    passed = 0
    failed = 0
    saw_pass_line = False
    saw_fail_line = False
    timing_seen: set[int] = set()
    test_timing_ms: dict[int, int] = {}
    total_timing_ms: int | None = None
    total_timing_seconds: str | None = None
    exception_lines: List[str] = []
    depth_assert_lines: List[str] = []
    current_class_test_num: int | None = None
    class_failed_tests: set[int] = set()
    summary_table_results: dict[int, str] = {}

    for line in lines:
        m = re.match(r"^\s*Passed:\s+(\d+)", line)
        if m:
            passed = int(m.group(1))
            saw_pass_line = True

        m = re.match(r"^\s*Failed:\s+(\d+)", line)
        if m:
            failed = int(m.group(1))
            saw_fail_line = True

        m = re.search(r"\[Timing\]\s*Test\s*(\d+)\s*:", line)
        if m:
            test_num = int(m.group(1))
            timing_seen.add(test_num)
            t = re.search(r"\[Timing\]\s*Test\s*\d+\s*:\s*(\d+)\s*ms", line)
            if t:
                test_timing_ms[test_num] = int(t.group(1))

        m = re.search(r"CLASS NAME:\s*Test\s*(\d+)", line)
        if m:
            current_class_test_num = int(m.group(1))

        m = re.search(r"STATUS:\s*(PASSED|FAILED)", line)
        if m and (current_class_test_num is not None):
            if m.group(1) == "FAILED":
                class_failed_tests.add(current_class_test_num)

        m = re.match(r"^\s*(\d+)\s*\|\s*(PASS|FAIL)\b", line)
        if m:
            summary_table_results[int(m.group(1))] = m.group(2)

        m = re.match(
            r"^\s*Total test execution time:\s*(\d+)\s*ms\s*\(([^\)]+)\)",
            line,
        )
        if m:
            total_timing_ms = int(m.group(1))
            total_timing_seconds = m.group(2).strip()

        if any(re.search(pattern, line) for pattern in EXCEPTION_PATTERNS):
            exception_lines.append(line)

        if any(re.search(pattern, line) for pattern in MAX_DEPTH_ASSERT_PATTERNS):
            depth_assert_lines.append(line)

    total = passed + failed
    summary_present = saw_pass_line and saw_fail_line
    timed_out = qemu_rc == 124
    early_assert_abort = qemu_rc == EARLY_ASSERT_ABORT_RC
    qemu_error = qemu_rc not in (0, 124, EARLY_ASSERT_ABORT_RC)
    complete_count = total == EXPECTED_TEST_COUNT
    clean_no_exceptions = len(exception_lines) == 0
    class_status_clean = len(class_failed_tests) == 0

    host_elapsed_ms = int(host_elapsed_seconds * 1000)
    print(
        "  Host wall-clock test duration: "
        f"{host_elapsed_ms} ms ({host_elapsed_seconds:.3f} s)"
    )

    if (
        summary_present
        and complete_count
        and (failed == 0)
        and (qemu_rc == 0)
        and clean_no_exceptions
        and class_status_clean
    ):
        print(f"  {arch} RESULT: ALL {passed} TESTS PASSED")
        return True

    if summary_present:
        print(f"  {arch} RESULT: FAILED ({passed}/{total} passed, {failed} failed)")
    elif timing_seen:
        print(
            f"  {arch} RESULT: FAILED "
            f"(summary unavailable; observed timing for {len(timing_seen)}/{EXPECTED_TEST_COUNT} tests)"
        )
    else:
        print(f"  {arch} RESULT: FAILED (summary unavailable; pass/fail counts unknown)")

    if timed_out:
        print("    Reason: QEMU timed out before completion")

    if early_assert_abort:
        print(
            "    Reason: QEMU stopped early after detecting "
            "max interrupt nest-depth ASSERT"
        )

    if depth_assert_lines:
        print(
            "    Reason: Max interrupt nest-depth ASSERT triggered "
            "(infinite recursion regression detected)"
        )

    if not class_status_clean:
        failed_list = ", ".join(f"{n:02d}" for n in sorted(class_failed_tests))
        print(f"    Reason: Per-test class status reported FAILED for test(s): {failed_list}")

        contradicted = [
            n
            for n in sorted(class_failed_tests)
            if summary_table_results.get(n) == "PASS"
        ]
        if contradicted:
            contradicted_list = ", ".join(f"{n:02d}" for n in contradicted)
            print(
                "    Reason: Summary table contradiction detected "
                f"(class FAILED but table PASS for test(s): {contradicted_list})"
            )

    if qemu_error:
        print(f"    Reason: QEMU exited with non-zero code {qemu_rc}")

    if not summary_present:
        print("    Reason: Test summary lines (Passed/Failed) were not found")

    if summary_present and not complete_count:
        print(
            "    Reason: Incomplete test run "
            f"(expected {EXPECTED_TEST_COUNT}, got {total})"
        )

    if timing_seen and (len(timing_seen) < EXPECTED_TEST_COUNT):
        print(
            "    Observed timing output for "
            f"{len(timing_seen)}/{EXPECTED_TEST_COUNT} tests"
        )

    if test_timing_ms:
        print("    Firmware-reported test timing summary:")
        print("      Note: Per-test timings come from firmware output, not host wall-clock")
        for test_num in sorted(test_timing_ms):
            print(f"      Test {test_num:02d}: {test_timing_ms[test_num]} ms")

    if total_timing_ms is not None:
        if total_timing_seconds is not None:
            print(
                "    Firmware-reported total test execution time: "
                f"{total_timing_ms} ms ({total_timing_seconds})"
            )
        else:
            print(f"    Firmware-reported total test execution time: {total_timing_ms} ms")

    if exception_lines:
        print("    Reason: Exception signatures found in firmware log")
        for line in exception_lines[:5]:
            print(f"      {line}")

    if depth_assert_lines:
        print("    Max-depth ASSERT signature(s):")
        for line in depth_assert_lines[:5]:
            print(f"      {line}")

    for line in lines:
        if re.search(r"ASSERT|FAILED|FAILURE MESSAGE", line):
            print(f"    {line}")

    return False


def select_platforms(platform: str) -> Iterable[str]:
    if platform == "Both":
        return ("AARCH64", "X64")
    return (platform,)


def run_windows(args: argparse.Namespace, edk2_root: Path) -> int:
    # Prefer CLANGPDB on Windows because it supports both X64 and AARCH64.
    toolchain = args.toolchain if args.toolchain else "CLANGPDB"

    print("\n=== DxeCoreTplTest Runner ===\n")
    print(f"Build mode: {'Clean' if args.clean else 'Incremental'}\n")

    targets = list(select_platforms(args.platform))
    qemu_x64 = resolve_qemu_executable("X64", windows_override=args.qemu_x64)
    qemu_aarch64 = resolve_qemu_executable("AARCH64", windows_override=args.qemu_aarch64)

    if not args.run_only:
        print("[BUILD]")
        for t in targets:
            dsc = "ArmVirtPkg/ArmVirtQemu.dsc" if t == "AARCH64" else "OvmfPkg/OvmfPkgX64.dsc"
            build_firmware_windows(edk2_root, t, dsc, toolchain=toolchain, clean=args.clean, verbose=args.verbose)
            build_test_app_windows(edk2_root, t, toolchain=toolchain, clean=args.clean, verbose=args.verbose)
        print()

    if args.build_only:
        return 0

    print("[STAGE]")
    if "AARCH64" in targets:
        stage_aarch64_windows(
            edk2_root,
            toolchain,
            args.verbose,
        )
    if "X64" in targets:
        stage_x64_windows(
            edk2_root,
            toolchain,
            args.verbose,
        )
    print("\n[RUN]")

    all_pass = True
    build_root = edk2_root / "Build"

    if "AARCH64" in targets:
        print(f"  Running AARCH64 QEMU with -singlestep (timeout: {args.timeout_seconds}s)...")
        armvirt_vvfat = "fat:rw:Build/ArmVirtTest"
        rc, host_elapsed = run_qemu(
            qemu_aarch64,
            [
                "-machine", "virt",
                "-cpu", "max",
                "-m", "256",
                "-singlestep",
                "-drive", "if=pflash,format=raw,file=Build\\QEMU_EFI.fd",
                "-drive", f"if=none,file={armvirt_vvfat},id=hd0,format=raw",
                "-device", "virtio-blk-device,drive=hd0",
                "-nic", "none",
                "-display", "none",
                "-serial", f"file:{build_root / 'aarch64_test_results.txt'}",
                "-monitor", "none",
            ],
            cwd=edk2_root,
            output_file=build_root / "aarch64_test_results.txt",
            arch="AARCH64",
            timeout_seconds=args.timeout_seconds,
            verbose=args.verbose,
        )
        if rc == 124:
            print(f"  TIMEOUT: QEMU did not exit within {args.timeout_seconds}s - killing (guest likely hung)")
        elif rc != 0:
            print(f"  ERROR: AARCH64 QEMU exited with code {rc}")
        all_pass = parse_results(
            build_root / "aarch64_test_results.txt",
            "AARCH64",
            rc,
            host_elapsed,
        ) and all_pass
        print()

    if "X64" in targets:
        shutil.copy2(edk2_root / f"Build/OvmfX64/DEBUG_{toolchain}/FV/OVMF_VARS.fd", build_root / "OVMF_VARS.fd")
        print(f"  Running X64 QEMU with -singlestep (timeout: {args.timeout_seconds}s)...")
        rc, host_elapsed = run_qemu(
            qemu_x64,
            [
                "-machine", "q35",
                "-m", "256",
                "-singlestep",
                "-drive", "if=pflash,format=raw,readonly=on,file=Build\\OVMF_CODE.fd",
                "-drive", "if=pflash,format=raw,file=Build\\OVMF_VARS.fd",
                "-drive", "file=fat:rw:Build\\OvmfTest,format=raw",
                "-nic", "none",
                "-display", "none",
                "-serial", f"file:{build_root / 'x64_test_results.txt'}",
                "-monitor", "none",
            ],
            cwd=edk2_root,
            output_file=build_root / "x64_test_results.txt",
            arch="X64",
            timeout_seconds=args.timeout_seconds,
            verbose=args.verbose,
        )
        if rc == 124:
            print(f"  TIMEOUT: QEMU did not exit within {args.timeout_seconds}s - killing (guest likely hung)")
        elif rc != 0:
            print(f"  ERROR: X64 QEMU exited with code {rc}")
        all_pass = parse_results(
            build_root / "x64_test_results.txt",
            "X64",
            rc,
            host_elapsed,
        ) and all_pass
        print()

    print("=== SUMMARY ===")
    if all_pass:
        print("ALL PLATFORMS PASSED")
        return 0
    print("SOME TESTS FAILED")
    return 1


def run_native(args: argparse.Namespace, edk2_root: Path) -> int:
    print("\n=== DxeCoreTplTest Runner (WSL) ===\n")
    print(f"Build mode: {'Clean' if args.clean else 'Incremental'}\n")

    targets = list(select_platforms(args.platform))
    toolchain = args.toolchain if args.toolchain else "GCC"
    qemu_x64 = resolve_qemu_executable("X64", posix_override=args.qemu_x64)
    qemu_aarch64 = resolve_qemu_executable("AARCH64", posix_override=args.qemu_aarch64)

    if not args.run_only:
        print("[BUILD]")
        for t in targets:
            dsc = "ArmVirtPkg/ArmVirtQemu.dsc" if t == "AARCH64" else "OvmfPkg/OvmfPkgX64.dsc"
            build_firmware_native(edk2_root, t, dsc, toolchain=toolchain, clean=args.clean, verbose=args.verbose)
            build_test_app_native(edk2_root, t, toolchain=toolchain, clean=args.clean, verbose=args.verbose)
        print()

    if args.build_only:
        return 0

    print("[STAGE]")
    if "AARCH64" in targets:
        stage_aarch64_native(edk2_root, toolchain)
    if "X64" in targets:
        stage_x64_native(edk2_root, toolchain)
    print("\n[RUN]")

    build_root = edk2_root / "Build"
    all_pass = True

    if "AARCH64" in targets:
        print(f"  Running AARCH64 QEMU with -singlestep (timeout: {args.timeout_seconds}s)...")
        armvirt_vvfat = f"fat:rw:{(build_root / 'ArmVirtTest').as_posix()}"
        rc, host_elapsed = run_qemu(
            qemu_aarch64,
            [
                "-machine", "virt",
                "-cpu", "max",
                "-m", "256",
                "-singlestep",
                "-drive", f"if=pflash,format=raw,file={build_root / 'QEMU_EFI.fd'}",
                "-drive", f"if=none,file={armvirt_vvfat},id=hd0,format=raw",
                "-device", "virtio-blk-device,drive=hd0",
                "-nic", "none",
                "-display", "none",
                "-serial", f"file:{build_root / 'aarch64_test_results.txt'}",
                "-monitor", "none",
            ],
            cwd=edk2_root,
            output_file=build_root / "aarch64_test_results.txt",
            arch="AARCH64",
            timeout_seconds=args.timeout_seconds,
            verbose=args.verbose,
        )
        if rc == 124:
            print(f"  TIMEOUT: AARCH64 QEMU exceeded {args.timeout_seconds}s")
        elif rc != 0:
            print(f"  ERROR: AARCH64 QEMU exited with {rc}")
        all_pass = parse_results(
            build_root / "aarch64_test_results.txt",
            "AARCH64",
            rc,
            host_elapsed,
        ) and all_pass
        print()

    if "X64" in targets:
        shutil.copy2(edk2_root / f"Build/OvmfX64/DEBUG_{toolchain}/FV/OVMF_VARS.fd", build_root / "OVMF_VARS.fd")
        print(f"  Running X64 QEMU with -singlestep (timeout: {args.timeout_seconds}s)...")
        rc, host_elapsed = run_qemu(
            qemu_x64,
            [
                "-machine", "q35",
                "-m", "256",
                "-singlestep",
                "-drive", f"if=pflash,format=raw,readonly=on,file={build_root / 'OVMF_CODE.fd'}",
                "-drive", f"if=pflash,format=raw,file={build_root / 'OVMF_VARS.fd'}",
                "-drive", f"file=fat:rw:{build_root / 'OvmfTest'},format=raw",
                "-nic", "none",
                "-display", "none",
                "-serial", f"file:{build_root / 'x64_test_results.txt'}",
                "-monitor", "none",
            ],
            cwd=edk2_root,
            output_file=build_root / "x64_test_results.txt",
            arch="X64",
            timeout_seconds=args.timeout_seconds,
            verbose=args.verbose,
        )
        if rc == 124:
            print(f"  TIMEOUT: X64 QEMU exceeded {args.timeout_seconds}s")
        elif rc != 0:
            print(f"  ERROR: X64 QEMU exited with {rc}")
        all_pass = parse_results(
            build_root / "x64_test_results.txt",
            "X64",
            rc,
            host_elapsed,
        ) and all_pass
        print()

    print("=== SUMMARY ===")
    if all_pass:
        print("ALL PLATFORMS PASSED")
        return 0
    print("SOME TESTS FAILED")
    return 1


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build and run DxeCoreTplTest")
    parser.add_argument("--platform", choices=["AARCH64", "X64", "Both"], default="Both")
    parser.add_argument("--build-only", action="store_true")
    parser.add_argument("--run-only", action="store_true")
    parser.add_argument("--clean", action="store_true")
    parser.add_argument("--verbose", "-v", action="store_true")
    parser.add_argument("--timeout-seconds", type=int, default=240)
    parser.add_argument(
        "--toolchain",
        default="",
        help="EDK II toolchain tag (Windows default: CLANGPDB, WSL/Linux default: GCC)",
    )
    parser.add_argument(
        "--qemu-x64",
        default="",
        help="Optional path or command name for qemu-system-x86_64",
    )
    parser.add_argument(
        "--qemu-aarch64",
        default="",
        help="Optional path or command name for qemu-system-aarch64",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    script_dir = Path(__file__).resolve().parent
    edk2_root = script_dir.parents[3]

    if args.build_only and args.run_only:
        print("ERROR: --build-only and --run-only cannot be used together")
        return 2

    if is_windows_host():
        return run_windows(args, edk2_root)
    return run_native(args, edk2_root)


if __name__ == "__main__":
    sys.exit(main())
