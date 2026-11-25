# @file test_HostBasedUnitTestRunner.py
#
# Tests for the HostBasedUnitTestRunner plugin.
#
# Run from the workspace root with:
#   python -m unittest discover -s BaseTools/Plugin/HostBasedUnitTestRunner/tests -v
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import os
import pathlib
import sys
import tempfile
import unittest
from types import SimpleNamespace
from unittest.mock import patch

test_file = pathlib.Path(__file__)
sys.path.append(str(test_file.parent.parent))

import HostBasedUnitTestRunner as runner_module  # noqa: E402


class FakeBuildEnvironment:
    def __init__(self, build_output_base):
        self.values = {
            "BUILD_OUTPUT_BASE": build_output_base,
            "CI_BUILD_TYPE": "host_unit_test",
            "CODE_COVERAGE": "FALSE",
            "TARGET_ARCH": "X64",
            "TOOL_CHAIN_TAG": "CLANGPDB",
        }

    def GetValue(self, key):
        return self.values.get(key)


class FakeShellEnvironment:
    def __init__(self):
        self.values = {}

    def set_shell_var(self, name, value):
        self.values[name] = value


class TestSingleModuleUnitTestRunner(unittest.TestCase):
    def run_module_test(self, build_output_base, module_base_name):
        builder = SimpleNamespace(
            env=FakeBuildEnvironment(build_output_base))
        runner = runner_module.HostBasedUnitTestRunner()

        with patch.object(
                runner_module,
                "GetHostInfo",
                return_value=SimpleNamespace(os="WINDOWS")), \
                patch.object(
                    runner_module.shell_environment,
                    "GetEnvironment",
                    return_value=FakeShellEnvironment()), \
                patch.object(runner_module, "RunCmd", return_value=0) as run_cmd:
            result = runner.run_tests(builder, module_base_name)

        return result, run_cmd

    def test_runs_only_selected_module(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            arch_dir = pathlib.Path(temp_dir) / "X64"
            arch_dir.mkdir()
            selected_test = arch_dir / "SelectedTest.exe"
            selected_test.touch()
            (arch_dir / "OtherTest.exe").touch()

            result, run_cmd = self.run_module_test(temp_dir, "SelectedTest")

            self.assertEqual(result, 0)
            run_cmd.assert_called_once_with(
                f'"{selected_test}"', "", workingdir=str(arch_dir))

    def test_missing_selected_module_does_not_run_other_tests(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            arch_dir = pathlib.Path(temp_dir) / "X64"
            arch_dir.mkdir()
            (arch_dir / "OtherTest.exe").touch()

            result, run_cmd = self.run_module_test(temp_dir, "SelectedTest")

            self.assertEqual(result, 1)
            run_cmd.assert_not_called()

    def test_runs_only_selected_module_on_linux(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            arch_dir = pathlib.Path(temp_dir) / "X64"
            arch_dir.mkdir()
            selected_test = arch_dir / "SelectedTest"
            selected_test.touch()
            (arch_dir / "OtherTest").touch()
            original_stat = os.stat

            def executable_stat(path):
                result = original_stat(path)
                if os.fspath(path) == str(selected_test):
                    mode = result.st_mode | runner_module.stat.S_IXUSR
                    result = os.stat_result((mode,) + tuple(result)[1:])
                return result

            builder = SimpleNamespace(
                env=FakeBuildEnvironment(temp_dir))
            runner = runner_module.HostBasedUnitTestRunner()
            with patch.object(
                    runner_module,
                    "GetHostInfo",
                    return_value=SimpleNamespace(os="LINUX")), \
                    patch.object(
                        runner_module.shell_environment,
                        "GetEnvironment",
                        return_value=FakeShellEnvironment()), \
                    patch.object(
                        runner_module,
                        "RunCmd",
                        return_value=0) as run_cmd, \
                    patch.object(
                        runner_module.os,
                        "stat",
                        side_effect=executable_stat):
                result = runner.run_tests(builder, "SelectedTest")

            self.assertEqual(result, 0)
            run_cmd.assert_called_once_with(
                f'"{selected_test}"', "", workingdir=str(arch_dir))


class TestLlvmToolPath(unittest.TestCase):
    def test_uses_clang_bin_on_windows(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            tool_path = pathlib.Path(temp_dir) / "llvm-profdata.exe"
            tool_path.touch()
            shell_env = SimpleNamespace(
                get_shell_var=lambda name: temp_dir)
            runner = runner_module.HostBasedUnitTestRunner()

            with patch.object(
                    runner_module.shell_environment,
                    "GetEnvironment",
                    return_value=shell_env), \
                    patch.object(
                        runner_module,
                        "GetHostInfo",
                        return_value=SimpleNamespace(os="WINDOWS")):
                result = runner._get_llvm_tool_path("llvm-profdata")

        self.assertEqual(result, str(tool_path))

    def test_falls_back_to_path_when_tool_missing_from_clang_bin(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            shell_env = SimpleNamespace(
                get_shell_var=lambda name: temp_dir)
            runner = runner_module.HostBasedUnitTestRunner()

            with patch.object(
                    runner_module.shell_environment,
                    "GetEnvironment",
                    return_value=shell_env), \
                    patch.object(
                        runner_module,
                        "GetHostInfo",
                        return_value=SimpleNamespace(os="WINDOWS")), \
                    self.assertLogs(level="WARNING"):
                result = runner._get_llvm_tool_path("llvm-profdata")

        self.assertEqual(result, "llvm-profdata")

    def test_falls_back_to_path_when_clang_bin_is_not_set(self):
        shell_env = SimpleNamespace(get_shell_var=lambda name: None)
        runner = runner_module.HostBasedUnitTestRunner()

        with patch.object(
                runner_module.shell_environment,
                "GetEnvironment",
                return_value=shell_env):
            result = runner._get_llvm_tool_path("llvm-profdata")

        self.assertEqual(result, "llvm-profdata")


if __name__ == "__main__":
    unittest.main()
