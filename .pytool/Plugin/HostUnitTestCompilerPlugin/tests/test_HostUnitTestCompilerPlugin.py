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
        class FakeEnvironment:
            def __init__(self):
                self.values = {
                    "TARGET_ARCH": "X64",
                    "TOOL_CHAIN_TAG": "CLANGPDB",
                    "BUILDMODULE": "UnitTestFrameworkPkg/Test/Selected.inf",
                }

            def GetValue(self, key, default=None):
                return self.values.get(key, default)

            def SetValue(self, key, value, reason):
                self.values[key] = value
                self.last_set_reason = reason
                return True

            def AllowOverride(self, key):
                self.allowed_override = key

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
                    compiler_module, "DscParser") as dsc_parser_class, \
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


if __name__ == "__main__":
    unittest.main()
