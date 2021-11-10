REM @file
REM
REM Compare VS2019 NOOPT package/platform builds
REM
REM Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
REM SPDX-License-Identifier: BSD-2-Clause-Patent
REM

..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p CryptoPkg/CryptoPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p CryptoPkg/Test/CryptoPkgHostUnitTest.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p EmbeddedPkg/EmbeddedPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p EmulatorPkg/EmulatorPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p FatPkg/FatPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p FmpDevicePkg/FmpDevicePkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p FmpDevicePkg/Test/FmpDeviceHostPkgTest.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p IntelFsp2Pkg/IntelFsp2Pkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p IntelFsp2WrapperPkg/IntelFsp2WrapperPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p MdeModulePkg/MdeModulePkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p MdePkg/MdePkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p MdePkg/Test/MdePkgHostTest.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p NetworkPkg/NetworkPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p OvmfPkg/OvmfPkgIa32.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p PcAtChipsetPkg/PcAtChipsetPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p RedfishPkg/RedfishPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p SecurityPkg/SecurityPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p ShellPkg/ShellPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p SignedCapsulePkg/SignedCapsulePkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p UefiCpuPkg/Test/UefiCpuPkgHostTest.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p SourceLevelDebugPkg/SourceLevelDebugPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p UefiCpuPkg/UefiCpuPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p UnitTestFrameworkPkg/UnitTestFrameworkPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -b NOOPT -p UnitTestFrameworkPkg/Test/UnitTestFrameworkPkgHostTest.dsc --quiet

..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p CryptoPkg/CryptoPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p CryptoPkg/Test/CryptoPkgHostUnitTest.dsc --quiet
REM 
REM Too many issues to fix right now
REM Build break EmbeddedPkg\Library\GdbSerialDebugPortLib\GdbSerialDebugPortLib.c(127): warning C4244: 'function': conversion from 'UINTN' to 'UINT32', possible loss of data
REM Build break EmbeddedPkg\Library\GdbSerialDebugPortLib\GdbSerialDebugPortLib.c(153): warning C4244: 'function': conversion from 'UINTN' to 'UINT32', possible loss of data
REM 
REM ..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p EmbeddedPkg/EmbeddedPkg.dsc --quiet
REM
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p EmulatorPkg/EmulatorPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p FatPkg/FatPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p FmpDevicePkg/FmpDevicePkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p FmpDevicePkg/Test/FmpDeviceHostPkgTest.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p IntelFsp2WrapperPkg/IntelFsp2WrapperPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p MdeModulePkg/MdeModulePkg.dsc --quiet
REM 
REM Skip because it uses cmocka that uses __LINE__ that can not be overriden
REM 
REM ..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p MdeModulePkg/Test/MdeModulePkgHostTest.dsc --quiet
REM
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p MdePkg/MdePkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p MdePkg/Test/MdePkgHostTest.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p NetworkPkg/NetworkPkg.dsc --quiet
REM
REM Skip because of Linux shell dependencies
REM
REM ..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p OvmfPkg/AmdSev/AmdSevX64.dsc --quiet
REM
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -a X64 -b NOOPT -p OvmfPkg/OvmfPkgIa32X64.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p OvmfPkg/OvmfPkgX64.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p OvmfPkg/OvmfXen.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p OvmfPkg/Bhyve/BhyveX64.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p OvmfPkg/Microvm/MicrovmX64.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p PcAtChipsetPkg/PcAtChipsetPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p RedfishPkg/RedfishPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p SecurityPkg/SecurityPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p ShellPkg/ShellPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p SignedCapsulePkg/SignedCapsulePkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p SourceLevelDebugPkg/SourceLevelDebugPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p UefiCpuPkg/UefiCpuPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p UefiCpuPkg/Test/UefiCpuPkgHostTest.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a IA32 -a X64 -b NOOPT -p UefiPayloadPkg/UefiPayloadPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p UnitTestFrameworkPkg/UnitTestFrameworkPkg.dsc --quiet
..\CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t VS2019 -j Build/Build.log -a X64 -b NOOPT -p UnitTestFrameworkPkg/Test/UnitTestFrameworkPkgHostTest.dsc --quiet
