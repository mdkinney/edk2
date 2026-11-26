# @file HostUnitTestCompilerPlugin.py
##
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import logging
import os
import re
import shutil
from io import StringIO
from edk2toollib.uefi.edk2.parsers.dsc_parser import DscParser
from edk2toollib.uefi.edk2.parsers.inf_parser import InfParser
from edk2toollib.utility_functions import GetHostInfo, RunCmd
from edk2toolext.environment.plugintypes.ci_build_plugin import ICiBuildPlugin
from edk2toolext.environment.uefi_build import UefiBuilder
from edk2toolext import edk2_logging
from edk2toolext.environment.var_dict import VarDict


class HostUnitTestCompilerPlugin(ICiBuildPlugin):
    """
    A CiBuildPlugin that compiles the dsc for host based unit test apps.
    An IUefiBuildPlugin may be attached to this plugin that will run the
    unit tests and collect the results after successful compilation.

    Configuration options:
    "HostUnitTestCompilerPlugin": {
        "DscPath": "<path to dsc from root of pkg>"
    }
    """

    def GetTestName(self, packagename: str, environment: VarDict) -> tuple:
        """ Provide the testcase name and classname for use in reporting
            testclassname: a descriptive string for the testcase can include whitespace
            classname: should be patterned <packagename>.<plugin>.<optionally any unique condition>

            Args:
              packagename: string containing name of package to build
              environment: The VarDict for the test to run in
            Returns:
                a tuple containing the testcase name and the classname
                (testcasename, classname)
        """
        num,types = self.__GetHostUnitTestArch(environment)
        types = types.replace(" ", "_")

        return ("Compile and Run Host-Based UnitTests for " + packagename + " on arch " + types,
                packagename + ".HostUnitTestCompiler." + types)

    def RunsOnTargetList(self):
        return ["NOOPT"]

    #
    # Find the intersection of application types that can run on this host
    # and the TARGET_ARCH being build in this request.
    #
    # return tuple with (number of UEFI arch types, space separated string)
    def __GetHostUnitTestArch(self, environment):
        requested = environment.GetValue("TARGET_ARCH").split(' ')
        host = []
        if GetHostInfo().arch == 'x86':
            #assume 64bit can handle 64 and 32
            #assume 32bit can only handle 32
            ## change once IA32 issues resolved host.append("IA32")
            if GetHostInfo().bit == '64':
                host.append("X64")
                # VS20xx IA32 builds have known issues using OpenCppCoverage
                # Only enable IA32 builds on 64bit hosts when using clang or gcc
                if not environment.GetValue("TOOL_CHAIN_TAG", "").startswith('VS'):
                    host.append("IA32")
        elif GetHostInfo().arch == 'ARM':
            if GetHostInfo().bit == '64':
                host.append("AARCH64")

        willrun = set(requested) & set(host)
        return (len(willrun), " ".join(willrun))

    def _run_single_module_tests(self, uefiBuilder, module_base_name):
        for plugin in uefiBuilder.pm.GetAllPlugins():
            if plugin.Module == "HostBasedUnitTestRunner":
                return plugin.Obj.run_tests(uefiBuilder, module_base_name)

        logging.info(
            "Host-Based Unit Test Runner is disabled; not running BUILDMODULE.")
        return None

    @staticmethod
    def _get_cl_version():
        cl_path = shutil.which("cl")
        if cl_path is None:
            return None

        output = StringIO()
        RunCmd(cl_path, "/Bv", outstream=output)
        version_match = re.search(
            r"\bCompiler Version\s+(\d+\.\d+)", output.getvalue())
        if version_match is None:
            return None

        return version_match.group(1)

    @staticmethod
    def _get_clang_version():
        clang_path = shutil.which("clang")
        if clang_path is None:
            return None

        output = StringIO()
        RunCmd(clang_path, "--version", outstream=output)
        version_match = re.search(
            r"\bclang version\s+(\d+(?:\.\d+)+)", output.getvalue())
        if version_match is None:
            return None

        return version_match.group(1)

    @staticmethod
    def _get_vs_clangpdb_compatibility_warning(environment):
        if environment.GetValue("TOOL_CHAIN_TAG", "").upper() != "CLANGPDB":
            return None

        cl_version = HostUnitTestCompilerPlugin._get_cl_version()
        if cl_version is None:
            return (
                "Could not determine the MSVC version with cl /Bv; "
                "skipping host-based unit tests."
            )

        clang_version = HostUnitTestCompilerPlugin._get_clang_version()
        if clang_version is None:
            return (
                "Could not determine the LLVM version with clang --version; "
                "skipping host-based unit tests."
            )

        cl_major, cl_minor = (int(part) for part in cl_version.split("."))
        clang_major = int(clang_version.split(".", maxsplit=1)[0])
        # MSVC 19.50 is the first toolset released with Visual Studio 2026.
        if (cl_major, cl_minor) >= (19, 50) and clang_major < 22:
            return (
                "MSVC {0} (VS2026 or later) requires LLVM 22 or later; "
                "skipping host-based unit tests for clang {1}.".format(
                    cl_version, clang_version)
            )

        return None

    ##
    # External function of plugin.  This function is used to perform the task of the ICiBuildPlugin Plugin
    #
    #   - package is the edk2 path to package.  This means workspace/packagepath relative.
    #   - edk2path object configured with workspace and packages path
    #   - PkgConfig Object (dict) for the pkg
    #   - EnvConfig Object
    #   - Plugin Manager Instance
    #   - Plugin Helper Obj Instance
    #   - Junit Logger
    #   - output_stream the StringIO output stream from this plugin via logging
    def RunBuildPlugin(self, packagename, Edk2pathObj, pkgconfig, environment, PLM, PLMHelper, tc, output_stream=None):
        self._env = environment
        environment.SetValue("CI_BUILD_TYPE", "host_unit_test", "Set in HostUnitTestCompilerPlugin")

        compatibility_warning = self._get_vs_clangpdb_compatibility_warning(
            environment)
        if compatibility_warning:
            logging.warning(compatibility_warning)
            tc.SetSkipped()
            return -1

        # Parse the config for required DscPath element
        if "DscPath" not in pkgconfig:
            tc.SetSkipped()
            tc.LogStdError("DscPath not found in config file.  Nothing to compile for HostBasedUnitTests.")
            return -1

        if pkgconfig["DscPath"].strip() == "":
            tc.SetSkipped()
            tc.LogStdError("Package HostBasedUnitTest Dsc not assigned.")
            return -1

        AP = Edk2pathObj.GetAbsolutePathOnThisSystemFromEdk2RelativePath(packagename)

        APDSC = os.path.join(AP, pkgconfig["DscPath"].strip())
        AP_Path = Edk2pathObj.GetEdk2RelativePathFromAbsolutePath(APDSC)
        if AP is None or AP_Path is None or not os.path.isfile(APDSC):
            tc.SetFailed("Package HostBasedUnitTest Dsc not found: {0}".format(AP_Path), "FILE_NOT_FOUND")
            tc.LogStdError("Package HostBasedUnitTest Dsc not found.")
            return 1

        logging.info("Building {0}".format(AP_Path))
        self._env.SetValue("ACTIVE_PLATFORM", AP_Path, "Set in Compiler Plugin")
        num, RUNNABLE_ARCHITECTURES = self.__GetHostUnitTestArch(environment)
        if(num == 0):
            tc.SetSkipped()
            tc.LogStdError("No host architecture compatibility")
            return -1

        if not environment.SetValue("TARGET_ARCH",
                                    RUNNABLE_ARCHITECTURES,
                                    "Update Target Arch based on Host Support"):
            #use AllowOverride function since this is a controlled attempt to change
            environment.AllowOverride("TARGET_ARCH")
            if not environment.SetValue("TARGET_ARCH",
                                        RUNNABLE_ARCHITECTURES,
                                        "Update Target Arch based on Host Support"):
                raise RuntimeError("Can't Change TARGET_ARCH as required")

        # Parse DSC to check for SUPPORTED_ARCHITECTURES
        dp = DscParser()
        dp.SetBaseAbsPath(Edk2pathObj.WorkspacePath)
        dp.SetPackagePaths(Edk2pathObj.PackagePathList)
        dp.ParseFile(AP_Path)
        if "SUPPORTED_ARCHITECTURES" in dp.LocalVars:
            SUPPORTED_ARCHITECTURES = dp.LocalVars["SUPPORTED_ARCHITECTURES"].split('|')
            TARGET_ARCHITECTURES = environment.GetValue("TARGET_ARCH").split(' ')

            # Skip if there is no intersection between SUPPORTED_ARCHITECTURES and TARGET_ARCHITECTURES
            if len(set(SUPPORTED_ARCHITECTURES) & set(TARGET_ARCHITECTURES)) == 0:
                tc.SetSkipped()
                tc.LogStdError("No supported architecutres to build for host unit tests")
                return -1

        build_module = (environment.GetValue("BUILDMODULE", "") or "").strip()
        module_base_name = None
        if build_module:
            module_path = Edk2pathObj.GetAbsolutePathOnThisSystemFromEdk2RelativePath(
                build_module)
            if module_path is None or not os.path.isfile(module_path):
                tc.SetFailed(
                    "Host unit test module not found: {0}".format(build_module),
                    "MODULE_NOT_FOUND")
                tc.LogStdError("Host unit test module not found.")
                return 1

            module_parser = InfParser().SetEdk2Path(Edk2pathObj)
            module_parser.ParseFile(build_module)
            if module_parser.Dict.get("MODULE_TYPE", "").upper() != "HOST_APPLICATION":
                tc.SetFailed(
                    "BUILDMODULE is not a host application: {0}".format(
                        build_module),
                    "NOT_HOST_APPLICATION")
                tc.LogStdError("BUILDMODULE must identify a host application.")
                return 1

            module_base_name = module_parser.Dict.get("BASE_NAME")
            if not module_base_name:
                tc.SetFailed(
                    "BASE_NAME not found in host test module: {0}".format(
                        build_module),
                    "BASE_NAME_NOT_FOUND")
                tc.LogStdError("BASE_NAME not found in host test module.")
                return 1

        uefiBuilder = UefiBuilder()
        # do all the steps
        # WorkSpace, PackagesPath, PInHelper, PInManager
        ret = uefiBuilder.Go(Edk2pathObj.WorkspacePath, os.pathsep.join(Edk2pathObj.PackagePathList), PLMHelper, PLM)
        if ret != 0:  # failure:
            tc.SetFailed("Compile failed for {0}".format(packagename), "Compile_FAILED")
            tc.LogStdError("{0} Compile failed with error code {1} ".format(AP_Path, ret))
            return 1

        if module_base_name is not None and uefiBuilder.SkipPostBuild:
            # Single-module builds skip the UEFI post-build plugins.
            ret = self._run_single_module_tests(uefiBuilder, module_base_name)
            if ret not in (None, 0):
                tc.SetFailed(
                    "Host unit test execution failed for {0}".format(
                        build_module),
                    "UNIT_TEST_FAILED")
                tc.LogStdError(
                    "Host unit test execution failed with error code {0}".format(
                        ret))
                return 1

        tc.SetSuccess()
        return 0
