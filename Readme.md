<!--- @file
  Readme.md for the EDK II Build Comparison Tools

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
-->

# EDK II Build Comparison Tools

This branch contains the `CompareBuild.py` script that can be used to determine
if two EDK II builds of the same package/platform are identical or not.

This branch also contains a manually triggered GitHub Action workflow that runs
the build comparison on all the packages and platforms in the edk2 repository
using the VS2019 and GCC5 tool chains for all the CPU architectures that each
platform/package/toolchain combination supports.

These tools can be used to verify that source format changes such as white space
changes, line ending changes, and code beautifiers such as `uncrustify` do not
introduce any functional changes to an EDKI II build. It can also be used to
verify changes to EDK II related build tools that are intended to be backwards
compatible.

# `CompareBuild.py` Python Script

This python script CompareBuild.py can be used to compare the results of two
different EDK II builds to determine if they generate the same binaries.

The CompareBuild.py command takes two git references (branch name, tag name, or
SHA) as input. Each git reference is checked out and the EDK II build command at
the end of the command line is executed. CompareBuild.py exits with success if
the builds are considered identical. An error code is returned if there are one
or more build comparison differences.

```
usage: CompareBuild [-h] --ref1 REF1 --ref2 REF2 [--skip-build] [--cleanup] [--verbose] ...

Compare two builds from different git branches/refs. Copyright (c) 2021, Intel Corporation. All rights reserved.

positional arguments:
  rest

optional arguments:
  -h, --help    show this help message and exit
  --ref1 REF1   First GIT branch/reference.
  --ref2 REF2   Second GIT branch/reference.
  --skip-build  Skip build and only redo compare from most recent build.
  --cleanup     Cleanup temporary Build and Conf folders.
  --verbose     Increase output messages
```

The following is a example of running `CompareBuild.py` to compare two different
builds of `FatPkg/FatPkg.dsc`. It shows that there were no file differences, 696
files from the build output directory were identical, and that one file was
ignored. The `--verbose` option can be used to output the names of all the files
compared.

```
edk2>CompareBuild.py --ref1 455b0347a7c5 --ref2 master build -a IA32 -n 5 -t VS2019 -b NOOPT -p FatPkg\FatPkg.dsc --quiet

WORKSPACE: C:\work\GitHub\tianocore\edk2
EDK_TOOLS_PATH: C:\work\GitHub\tianocore\edk2\BaseTools
Build Directory: C:\work\GitHub\tianocore\edk2\Build
Checkout: 455b0347a7c5
Run: build -a IA32 -n 5 -t VS2019 -b NOOPT -p FatPkg\FatPkg.dsc --quiet --conf=C:\work\GitHub\tianocore\edk2\Build\Conf.temp
Build environment: Windows-10-10.0.18362-SP0
Build start time: 09:11:43, Nov.15 2021

WORKSPACE        = c:\work\github\tianocore\edk2
EDK_TOOLS_PATH   = c:\work\github\tianocore\edk2\basetools
EDK_TOOLS_BIN    = c:\work\github\tianocore\edk2\basetools\bin\win32
CONF_PATH        = C:\work\GitHub\tianocore\edk2\Build\Conf.temp
PYTHON_COMMAND   = py -3

Processing meta-data .. done!
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\PeimEntryPoint\PeimEntryPoint.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\PeiServicesTablePointerLib\PeiServicesTablePointerLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\BasePcdLibNull\BasePcdLibNull.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\BaseMemoryLib\BaseMemoryLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\PeiMemoryAllocationLib\PeiMemoryAllocationLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\BaseLib\BaseLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\BaseDebugLibNull\BaseDebugLibNull.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\PeiServicesLib\PeiServicesLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\RegisterFilterLibNull\RegisterFilterLibNull.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\PeiHobLib\PeiHobLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\UefiDriverEntryPoint\UefiDriverEntryPoint.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\UefiLib\UefiLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\UefiRuntimeServicesTableLib\UefiRuntimeServicesTableLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\UefiDevicePathLib\UefiDevicePathLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\UefiMemoryAllocationLib\UefiMemoryAllocationLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\BasePrintLib\BasePrintLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\UefiBootServicesTableLib\UefiBootServicesTableLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\FatPkg\EnhancedFatDxe\Fat.inf [IA32]
Building ... c:\work\github\tianocore\edk2\FatPkg\FatPei\FatPei.inf [IA32]

- Done -
Build end time: 09:12:03, Nov.15 2021
Build total time: 00:00:19

Checkout: master
Run: build -a IA32 -n 5 -t VS2019 -b NOOPT -p FatPkg\FatPkg.dsc --quiet --conf=C:\work\GitHub\tianocore\edk2\Build\Conf.temp
Build environment: Windows-10-10.0.18362-SP0
Build start time: 09:12:09, Nov.15 2021

WORKSPACE        = c:\work\github\tianocore\edk2
EDK_TOOLS_PATH   = c:\work\github\tianocore\edk2\basetools
EDK_TOOLS_BIN    = c:\work\github\tianocore\edk2\basetools\bin\win32
CONF_PATH        = C:\work\GitHub\tianocore\edk2\Build\Conf.temp
PYTHON_COMMAND   = py -3

Processing meta-data .. done!
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\PeimEntryPoint\PeimEntryPoint.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\PeiServicesTablePointerLib\PeiServicesTablePointerLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\BasePcdLibNull\BasePcdLibNull.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\BaseMemoryLib\BaseMemoryLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\PeiMemoryAllocationLib\PeiMemoryAllocationLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\BaseLib\BaseLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\BaseDebugLibNull\BaseDebugLibNull.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\PeiServicesLib\PeiServicesLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\RegisterFilterLibNull\RegisterFilterLibNull.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\PeiHobLib\PeiHobLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\UefiDriverEntryPoint\UefiDriverEntryPoint.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\UefiLib\UefiLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\UefiRuntimeServicesTableLib\UefiRuntimeServicesTableLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\UefiDevicePathLib\UefiDevicePathLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\UefiMemoryAllocationLib\UefiMemoryAllocationLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\BasePrintLib\BasePrintLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\MdePkg\Library\UefiBootServicesTableLib\UefiBootServicesTableLib.inf [IA32]
Building ... c:\work\github\tianocore\edk2\FatPkg\EnhancedFatDxe\Fat.inf [IA32]
Building ... c:\work\github\tianocore\edk2\FatPkg\FatPei\FatPei.inf [IA32]

- Done -
Build end time: 09:12:27, Nov.15 2021
Build total time: 00:00:18

Checkout: master
PASS: Builds identical: build -a IA32 -n 5 -t VS2019 -b NOOPT -p FatPkg\FatPkg.dsc --quiet
  Number same      = 696
  Number ignored   = 1
  Number different = 0
```

## EDK II Build Comparison Overview

The approach to comparing EDK II builds is to compare the build output
directories after running the same build command on two different versions of
the EDK II sources. The EDK II build tools already support deterministic builds
for the FFS, FV, and FD images generated by a platform build. However, a
platform build only contains a subset of the functions that are in the EDK II
source tree. In order to guarantee that two builds of the EDK II source are
considered identical, the comparison needs to be extended to the OBJ, LIB, and
DLL files in the build output directory.

The OBJ, LIB, and DLL files may show differences based on the behavior of the
compiler and linker. There has been significant work on the concepts of
deterministic and reproducible builds. Some background and details on these
topics can be found here:

* https://reproducible-builds.org/
* https://blog.conan.io/2019/09/02/Deterministic-builds-with-C-C++.html

## Handling Time Stamps and Date Stamps

1) Use of `__TIME__` and `__DATE__` in C source files.
   * Append the following defines to `CC_FLAGS` and `ASLCC_FLAGS` for GCC family builds.
     Apply this update to a copy of `build_rule.template` to a new `Conf` directory.

     `-D__TIME__=\'"00:00:00"\' -D__DATE__=\'"Jan 1 2021"\' -Wno-builtin-macro-redefined`

   * Append the following defines to `CC_FLAGS` and `ASLCC_FLAGS` for MSFT family builds.
     Apply this update to a copy of `build_rule.template` to a new `Conf` directory.

     `-D__TIME__="00:00:00" -D__DATE__="Jan 1 2021" /wd4117`

2) Time and date stamps generated by compilers/assemblers/linkers in OBJ, LIB,
   and DLL output files. The VS2019 tool chain supports the `-Brepo` flag to
   replace time and date stamps in OBJ, LIB, DLL files with a constant value.
   The NASM assembler 2.15.05 and above supports the `--reproducible` option to
   replace time and date stamps in OBJ files with a constant value.

   * Append the option to `CC_FLAGS` and `ASLCC_FLAGS` for MSFT family builds.
     Apply this update to a copy of `build_rule.template` to a new `Conf` directory.

     `-Brepo`

   * Append the following option to `ASM_FLAGS` for MSFT family builds.
     Apply this update to a copy of `build_rule.template` to a new `Conf` directory.

     `-Brepo`

   * Append the following option to `NASM_FLAGS` for MSFT family and GCC family builds.
     Apply this update to a copy of `build_rule.template` to a new `Conf` directory.

     `-Brepoducible`

   * Append the following option to `SLINK_FLAGS` for MSFT family builds.
     Apply this update to a copy of `build_rule.template` to a new `Conf` directory.

     `-Brepo`

   * Append the following option to `DLINK_FLAGS` and `ASLDLINK_FLAGS` for MSFT family builds.
     Apply this update to a copy of `build_rule.template` to a new `Conf` directory.

     `-Brepo`

3) Time and date stamps generated in GCC5 linker output files
   * Update the `SLINK_FLAGS` for GCC family builds to use 'crD' instead of `cr`.
     Apply this update to a copy of `build_rule.template` to a new `Conf` directory.

4) Compiler use of random numbers in symbol generation
   * Append the following defines to `CC_FLAGS` for GCC builds.
     Apply this update to a copy of `build_rule.template` to a new `Conf` directory.

     `-frandom-seed=CONSTANTSEEDSTRING`

5) There is one module that defines `BUILD_EPOCH` based on the build systems
   time and date when the EDK II build command is executed. This is similar to
   use of `__TIME__` and `__DATE__` in C source files. The solution is to define
   `BUILD_EPOCH` to a constant integer value of 1. Since `BUILD_EPOCH` may
   already be defined, the solution is to  undefine `BUILD_EPOCH` and define
   `BUILD_EPOCH` to a value of 0.
   * Append the following defines to `CC_FLAGS` for GCC and MSFT family builds.
     Apply this update to a copy of `build_rule.template` to a new `Conf` directory.

     `-UBUILD_EPOCH -DBUILD_EPOCH=0`

## Handling Line Number Differences

The goal of deterministic builds is to make sure the build output is the same
when the same sources are used on different build systems. The goal of the
`CompareBuild.py` tool has an extra requirement to determine if the same
binaries are generated even if formatting changes are made to the source files.
Source format changes may change the line numbers. Line number information
is present in builds from the following sources:

* Use of the `__LINE__` macro in C code
* `#line` statements generated by a C preprocessor
* Compiler generated debug information that captures association between the
  line number of C source file and the generated binary.
* Use of link time optimization features that put add extra information in
  OBJ files (including source file line number information).

In order to perform build comparison, the sources of line number information
differences must be eliminated. The following techniques are applied:

1) Only use the NOOPT edk2 build target.
   * DEBUG build targets enable debug information and link time optimization
     that contains line number information
   * RELEASE build targets enable link time optimization that contains line
     number information.
2) Disable the generation of debug information in NOOPT builds.
   * For VS2019 tool chains, this requires removing the `/Zi`, `/Zd`, `/Z7`,
     and `/ZI` options from `CC_FLAGS`. Unfortunately, VS2019 does not support
     a way to disable these options once they are set, so the only method to
     remove these options is to remove them from `tools_def.txt`. A new copy of
     `tools_def.template` is made into a new `Conf.temp` directory and these
     4 debug information options are removed from `CC_FLAGS` statements that use
     them.
   * For GCC5 builds, the `-g0` option is appended to the end of `CC_FLAGS`
     overriding any other `-g` options. A new copy of `build_rule.template` is
     made into a new `Conf.temp` directory and `-g0` is appended after the use
     of `CC_FLAGS` in GCC family build rules.
3) Disable the use of the `-l` option in the VFR Compiler. This option enables
   the generation of a VFR listing files that contains line number information.
   Unfortunately, the VFR Compiler does not support a way to disable this
   option, so the only method to remove this option is to remove them from
   `tools_def.txt`. A new copy of `tools_def.template` is made into a new
   `Conf` directory and the `-l` options is removed from `VFR_FLAGS` statements.
4) Define `DEBUG_LINE_NUMBER` to a constant string value.
   * Append the following define to `CC_FLAGS` for MSFT and GCC families.
     Apply this update to a copy of `build_rule.template` to a new `Conf` directory.

     `-DDEBUG_LINE_NUMBER=1`

## Handling `ASSERT()` Expression Differences

In some cases, the expression passed into an `ASSERT()` macro may have white
space changes as part of code beautification processing. The addition or removal
of white space in this use case, changes the values of read-only ASCII strings
in builds that enable `ASSERT()` macros. In order to address these changes in
these expression strings, the following techniques are applied:

1) Define `DEBUG_EXPRESSION_STRING_VALUE` to a constant string value.
   * Append the following define to `CC_FLAGS` for MSFT and GCC builds.
     Apply this update to a copy of `build_rule.template` to a new `Conf` directory.

     `-DDEBUG_EXPRESSION_STRING_VALUE=\'"Hello"\'`

## Additional Constraints

1) The VS2019 tool chain supports **IA32**, **X64**, **ARM**, and **AARCH64**.
   However, there are some elements missing that prevent **ARM** and **AARCH64**
   builds from working for all packages/platforms in the edk2 repo. As a result,
   the VS2019 builds will focus on **IA32** and **X64**, and the GCC5 builds
   will focus on **IA32**, **X64**, **ARM**, **AARCH64**, and **RISCV64**.
2) Host based unit tests using the `cmocka` library features in the
   `UnitTestFrameworkPkg` use the `__LINE__` macro to identify the unit test
   source line number if a unit test assertion is triggered. `cmocka` is
   included as a submodule, so there is no method to change the behavior of the
   `cmocka` unit test assertion macros to replace the use of `__LINE__` with a
   constant value.

## Comparing Build Output Directories

There are a few files that are generated in the build output directory that have
differences that can be safely ignored. These include:

1) `Build.log` files that contains time stamps
2) `GlobalVar_*.bin` files that contain a time stamp from the EDK II build tools
   to support incremental builds.
3) `*PcdValueInit` directory that contains applications generated and built by
   the EDK II build tools to determine the default values of Structured PCDs.
   These applications are compiled using flags that include time/date stamps in
   the executable and there is no mechanism to override these flags. The files
   in this folder can be safely ignored because the data generated by the
   generated application is included in the generated FW image that is compared.
4) `*.i`, `*.ii`, `*.iii`, `*.iiii` files that are intermediate build files that
   are typically output from a C preprocessor operation that may contain `#line`
   statements.

# `CompareEdk2Build.yml` GitHub Action

The GitHub Action Workflow file in `.github/workflow/CompareEdk2Build.yml` uses
the `CompareBuild.py` script to perform build comparison for all the supported
VS2019 and GCC5 package and platform builds in an edk2 repository. The edk2
repository and the two references from that library can be configured when this
manual workflow is triggered. It should work on the main edk2 repository and any
of the forks of the edk2 repository. There are 30 VS2019 builds and 40 GCC5
builds.

## Known Limitations

There are a few packages/platforms that are not supported:

* VS2019
  * `EmbeddedPkg/EmbeddedPkg.dsc`: Too many build issues that need to be resolved.
  * `MdeModulePkg/Test/MdeModulePkgHostTest.dsc`: Uses `__LINE__` in `cmocka` macros.
  * `OvmfPkg/AmdSev/AmdSevX64.dsc`: Has linux shell dependencies.
* GCC5
  * `EmulatorPkg/EmulatorPkg.dsc`: Depends on X11 which is difficult to support in CI.
  * `MdeModulePkg/MdeModulePkg.dsc` (ARM): HII resource section link issues related to
    use of `objcopy` to create an obj with incorrect ABI.
  * `MdeModulePkg/Test/MdeModulePkgHostTest.dsc`: Uses `__LINE__` in cmocka macros.
  * `OvmfPkg/AmdSev/AmdSevX64.dsc`: Has linux shell dependencies.
