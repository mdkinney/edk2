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


class TestClangGenProfdata(unittest.TestCase):
    def run_profdata(self, temp_dir, merge_result=0):
        runner = runner_module.HostBasedUnitTestRunner()
        profraw_file = pathlib.Path(temp_dir) / "test.profraw"
        profraw_file.touch()
        calls = []

        def fake_run_cmd(command, arguments):
            calls.append(arguments)
            if arguments.startswith("merge"):
                return merge_result
            self.fail(f"Unexpected llvm-profdata command: {arguments}")

        with patch.object(
                runner,
                "_get_llvm_tool_path",
                return_value="llvm-profdata"), \
                patch.object(
                    runner_module,
                    "RunCmd",
                    side_effect=fake_run_cmd):
            result = runner.clang_gen_profdata(
                temp_dir,
                os.path.join(temp_dir, "profrawlist.txt"),
                os.path.join(temp_dir, "merged.profdata"),
            )

        return result, calls

    def test_returns_success_when_merge_succeeds(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            result, calls = self.run_profdata(temp_dir)

        self.assertEqual(result, 0)
        self.assertEqual(len(calls), 1)
        self.assertTrue(calls[0].startswith("merge -sparse "))

    def test_returns_error_when_merge_fails(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            result, calls = self.run_profdata(temp_dir, merge_result=1)

        self.assertEqual(result, 1)
        self.assertEqual(len(calls), 1)

class TestClangProfileDataTotalCount(unittest.TestCase):
    def check_profile(self, show_output, command_result=0):
        runner = runner_module.HostBasedUnitTestRunner()

        def fake_run_cmd(command, arguments, outstream=None, logging_level=None):
            self.assertEqual(logging_level, runner_module.logging.DEBUG)
            if outstream is not None:
                outstream.write(show_output)
            return command_result

        with patch.object(
                runner,
                "_get_llvm_tool_path",
                return_value="llvm-profdata"), \
                patch.object(
                    runner_module,
                    "RunCmd",
                    side_effect=fake_run_cmd):
            return runner.clang_prof_data_get_total_count(
                "merged.profdata")

    def test_returns_total_count(self):
        result = self.check_profile(
            "Functions shown: 2\nTotal functions: 2\nTotal count: 42\n")

        self.assertEqual(result, 42)

    def test_returns_zero_when_profile_has_no_counts(self):
        result = self.check_profile(
            "Functions shown: 2\nTotal functions: 2\nTotal count: 0\n")

        self.assertEqual(result, 0)

    def test_returns_none_when_total_count_is_missing(self):
        result = self.check_profile("Functions shown: 1\n")

        self.assertIsNone(result)

    def test_returns_none_when_inspection_command_fails(self):
        result = self.check_profile("", command_result=1)

        self.assertIsNone(result)


class TestClangCoverageNoData(unittest.TestCase):
    def test_empty_profile_skips_lcov_without_failure(self):
        runner = runner_module.HostBasedUnitTestRunner()
        with tempfile.TemporaryDirectory() as temp_dir:
            builder = SimpleNamespace(
                env=FakeBuildEnvironment(temp_dir))
            builder.env.values["WORKSPACE"] = temp_dir
            test_file = pathlib.Path(temp_dir) / "SampleTest.exe"
            test_file.touch()
            (pathlib.Path(temp_dir) / "test.profraw").touch()
            with patch.object(
                    runner_module,
                    "GetHostInfo",
                    return_value=SimpleNamespace(os="WINDOWS")), \
                    patch.object(
                        runner,
                        "clang_gen_profdata",
                        return_value=0), \
                    patch.object(
                        runner,
                        "clang_prof_data_get_total_count",
                        return_value=0) as get_total_count, \
                    patch.object(runner, "clang_gen_lcov_xml") as gen_lcov_xml:
                result = runner.gen_code_coverage_clang(
                    builder, [str(test_file)])

        self.assertEqual(result, 0)
        self.assertEqual(get_total_count.call_count, 2)
        gen_lcov_xml.assert_not_called()


if __name__ == "__main__":
    unittest.main()
