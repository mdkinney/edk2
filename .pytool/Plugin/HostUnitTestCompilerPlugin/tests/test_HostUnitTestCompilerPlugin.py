# @file test_HostUnitTestCompilerPlugin.py
#
# Tests for the HostUnitTestCompilerPlugin.
#
# Run from the workspace root with:
#   python -m unittest discover -s .pytool/Plugin/HostUnitTestCompilerPlugin/tests -v
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import pathlib
import sys
import tempfile
import unittest
from types import SimpleNamespace
from unittest.mock import Mock, patch

test_file = pathlib.Path(__file__)
sys.path.append(str(test_file.parent.parent))

import HostUnitTestCompilerPlugin as compiler_module  # noqa: E402


class FakeEnvironment:
    def __init__(self, values=None):
        self.values = {
            "TARGET_ARCH": "X64",
            "TOOL_CHAIN_TAG": "CLANGPDB",
            "BUILDMODULE": "UnitTestFrameworkPkg/Test/Selected.inf",
        }
        if values:
            self.values.update(values)

    def GetValue(self, key, default=None):
        return self.values.get(key, default)

    def SetValue(self, key, value, reason):
        self.values[key] = value
        self.last_set_reason = reason
        return True

    def AllowOverride(self, key):
        self.allowed_override = key


class TestSingleModuleHostTestDispatch(unittest.TestCase):
    def test_dispatches_selected_module_to_host_test_runner(self):
        test_runner = Mock()
        test_runner.run_tests.return_value = 1
        plugin_descriptor = SimpleNamespace(
            Module="HostBasedUnitTestRunner",
            Obj=test_runner)
        uefi_builder = SimpleNamespace(
            pm=SimpleNamespace(GetAllPlugins=lambda: [plugin_descriptor]))

        plugin = compiler_module.HostUnitTestCompilerPlugin()
        result = plugin._run_single_module_tests(
            uefi_builder, "SampleGoogleTestHost")

        self.assertEqual(result, 1)
        test_runner.run_tests.assert_called_once_with(
            uefi_builder, "SampleGoogleTestHost")

    def test_build_plugin_runs_selected_module_after_build(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            workspace = pathlib.Path(temp_dir)
            package_dir = workspace / "UnitTestFrameworkPkg"
            module_dir = package_dir / "Test"
            module_dir.mkdir(parents=True)
            (package_dir / "HostTest.dsc").touch()
            (module_dir / "Selected.inf").touch()

            edk2_path = SimpleNamespace(
                WorkspacePath=temp_dir,
                PackagePathList=[temp_dir],
                GetAbsolutePathOnThisSystemFromEdk2RelativePath=lambda path:
                    str(workspace / path),
                GetEdk2RelativePathFromAbsolutePath=lambda path:
                    str(pathlib.Path(path).relative_to(workspace)))
            inf_parser = Mock()
            inf_parser.Dict = {
                "MODULE_TYPE": "HOST_APPLICATION",
                "BASE_NAME": "SelectedTest",
            }
            inf_parser.SetEdk2Path.return_value = inf_parser

            test_runner = Mock()
            test_runner.run_tests.return_value = 0
            plugin_descriptor = SimpleNamespace(
                Module="HostBasedUnitTestRunner",
                Obj=test_runner)
            uefi_builder = Mock()
            uefi_builder.Go.return_value = 0
            uefi_builder.SkipPostBuild = True
            uefi_builder.pm.GetAllPlugins.return_value = [plugin_descriptor]
            test_case = Mock()

            with patch.object(
                    compiler_module.HostUnitTestCompilerPlugin,
                    "_get_cl_version",
                    return_value="19.50"), \
                    patch.object(
                        compiler_module.HostUnitTestCompilerPlugin,
                        "_get_clang_version",
                        return_value="22.1.8"), \
                    patch.object(
                        compiler_module,
                        "DscParser") as dsc_parser_class, \
                    patch.object(
                        compiler_module,
                        "InfParser",
                        return_value=inf_parser), \
                    patch.object(
                        compiler_module,
                        "GetHostInfo",
                        return_value=SimpleNamespace(arch="x86", bit="64")), \
                    patch.object(
                        compiler_module,
                        "UefiBuilder",
                        return_value=uefi_builder):
                dsc_parser_class.return_value.LocalVars = {}
                plugin = compiler_module.HostUnitTestCompilerPlugin()
                result = plugin.RunBuildPlugin(
                    "UnitTestFrameworkPkg",
                    edk2_path,
                    {"DscPath": "HostTest.dsc"},
                    FakeEnvironment(),
                    None,
                    None,
                    test_case)

        self.assertEqual(result, 0)
        test_runner.run_tests.assert_called_once_with(
            uefi_builder, "SelectedTest")
        test_case.SetSuccess.assert_called_once()

    def test_vs2026_clangpdb_with_llvm_below_22_is_skipped(self):
        plugin = compiler_module.HostUnitTestCompilerPlugin()
        environment = FakeEnvironment()
        test_case = Mock()

        with patch.object(
                compiler_module.HostUnitTestCompilerPlugin,
                "_get_cl_version",
                return_value="19.50"), \
                patch.object(
                    compiler_module.HostUnitTestCompilerPlugin,
                    "_get_clang_version",
                    return_value="21.1.8"), \
                patch.object(
                    compiler_module, "UefiBuilder") as uefi_builder, \
                self.assertLogs(level="WARNING") as log:
            result = plugin.RunBuildPlugin(
                "UnitTestFrameworkPkg",
                None,
                {},
                environment,
                None,
                None,
                test_case)

        self.assertEqual(result, -1)
        test_case.SetSkipped.assert_called_once()
        test_case.SetFailed.assert_not_called()
        test_case.LogStdError.assert_not_called()
        uefi_builder.assert_not_called()
        self.assertIn("requires LLVM 22 or later", log.output[0])

    def test_msvc_before_vs2026_with_llvm_below_22_is_compatible(self):
        plugin = compiler_module.HostUnitTestCompilerPlugin()

        with patch.object(
                compiler_module.HostUnitTestCompilerPlugin,
                "_get_cl_version",
                return_value="19.44"), \
                patch.object(
                    compiler_module.HostUnitTestCompilerPlugin,
                    "_get_clang_version",
                    return_value="21.1.8"):
            warning = plugin._get_vs_clangpdb_compatibility_warning(
                FakeEnvironment())

        self.assertIsNone(warning)

    def test_compatibility_check_reads_cl_and_clang_versions(self):
        plugin = compiler_module.HostUnitTestCompilerPlugin()
        command_output = {
            "/Bv": (
                "Microsoft (R) C/C++ Optimizing Compiler "
                "Version 19.50.35717 for x64\n"
            ),
            "--version": "clang version 21.1.8\n",
        }
        commands = []
        which_calls = []

        def run_command(command, arguments, outstream=None):
            commands.append((command, arguments))
            outstream.write(command_output[arguments])
            return 0

        def find_command(command):
            which_calls.append(command)
            return command

        with patch.object(
                compiler_module.shutil,
                "which",
                side_effect=find_command), \
                patch.object(
                    compiler_module, "RunCmd", side_effect=run_command):
            warning = plugin._get_vs_clangpdb_compatibility_warning(
                FakeEnvironment())

        self.assertEqual(which_calls, ["cl", "clang"])
        self.assertEqual(commands, [("cl", "/Bv"), ("clang", "--version")])
        self.assertIn("clang 21.1.8", warning)


if __name__ == "__main__":
    unittest.main()
