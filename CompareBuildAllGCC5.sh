## @file
# Compare GCC5 NOOPT package/platform builds
#
# Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a ARM -b NOOPT -p ArmPkg/ArmPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a AARCH64 -b NOOPT -p ArmPkg/ArmPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a ARM -b NOOPT -p ArmPlatformPkg/ArmPlatformPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a AARCH64 -b NOOPT -p ArmPlatformPkg/ArmPlatformPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a AARCH64 -b NOOPT -p ArmVirtPkg/ArmVirtCloudHv.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a ARM -b NOOPT -p ArmVirtPkg/ArmVirtCloudHv.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a AARCH64 -b NOOPT -p ArmVirtPkg/ArmVirtQemu.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a ARM -b NOOPT -p ArmVirtPkg/ArmVirtQemu.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a AARCH64 -b NOOPT -p ArmVirtPkg/ArmVirtQemuKernel.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a ARM -b NOOPT -p ArmVirtPkg/ArmVirtQemuKernel.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a AARCH64 -b NOOPT -p ArmVirtPkg/ArmVirtXen.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a ARM -b NOOPT -p ArmVirtPkg/ArmVirtXen.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p CryptoPkg/CryptoPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p CryptoPkg/CryptoPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a ARM -b NOOPT -p CryptoPkg/CryptoPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a AARCH64 -b NOOPT -p CryptoPkg/CryptoPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a RISCV64 -b NOOPT -p CryptoPkg/CryptoPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p CryptoPkg/Test/CryptoPkgHostUnitTest.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p CryptoPkg/Test/CryptoPkgHostUnitTest.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a ARM -b NOOPT -p DynamicTablesPkg/DynamicTablesPkg.dsc --quiet    
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a AARCH64 -b NOOPT -p DynamicTablesPkg/DynamicTablesPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p EmbeddedPkg/EmbeddedPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p EmbeddedPkg/EmbeddedPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a ARM -b NOOPT -p EmbeddedPkg/EmbeddedPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a AARCH64 -b NOOPT -p EmbeddedPkg/EmbeddedPkg.dsc --quiet
#
# Skip because X11 not supported on Ubuntu-20.04
#
#python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p EmulatorPkg/EmulatorPkg.dsc --quiet  
#python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p EmulatorPkg/EmulatorPkg.dsc --quiet 
#
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p FatPkg/FatPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p FatPkg/FatPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a ARM -b NOOPT -p FatPkg/FatPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a AARCH64 -b NOOPT -p FatPkg/FatPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a RISCV64 -b NOOPT -p FatPkg/FatPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p FmpDevicePkg/FmpDevicePkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p FmpDevicePkg/FmpDevicePkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a ARM -b NOOPT -p FmpDevicePkg/FmpDevicePkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a AARCH64 -b NOOPT -p FmpDevicePkg/FmpDevicePkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a RISCV64 -b NOOPT -p FmpDevicePkg/FmpDevicePkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p FmpDevicePkg/Test/FmpDeviceHostPkgTest.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p FmpDevicePkg/Test/FmpDeviceHostPkgTest.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p IntelFsp2Pkg/IntelFsp2Pkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p IntelFsp2WrapperPkg/IntelFsp2WrapperPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p IntelFsp2WrapperPkg/IntelFsp2WrapperPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p MdeModulePkg/MdeModulePkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p MdeModulePkg/MdeModulePkg.dsc --quiet
#
# Skip because HII resource section link failure on Linux GCC ARM NOOPT if HII obj is not last in list of linked libs.
#
# python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a ARM -b NOOPT -p MdeModulePkg/MdeModulePkg.dsc --quiet
#
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a AARCH64 -b NOOPT -p MdeModulePkg/MdeModulePkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a RISCV64 -b NOOPT -p MdeModulePkg/MdeModulePkg.dsc --quiet
#
# Skip because it uses cmocka that uses __LINE__ that can not be overriden
#
# python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p MdeModulePkg/Test/MdeModulePkgHostTest.dsc --quiet
#
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p MdePkg/MdePkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p MdePkg/MdePkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a ARM -b NOOPT -p MdePkg/MdePkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a AARCH64 -b NOOPT -p MdePkg/MdePkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a RISCV64 -b NOOPT -p MdePkg/MdePkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p MdePkg/Test/MdePkgHostTest.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p MdePkg/Test/MdePkgHostTest.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p NetworkPkg/NetworkPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p NetworkPkg/NetworkPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a ARM -b NOOPT -p NetworkPkg/NetworkPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a AARCH64 -b NOOPT -p NetworkPkg/NetworkPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a RISCV64 -b NOOPT -p NetworkPkg/NetworkPkg.dsc --quiet
#
# Skip because "Can't find grub mkimage"
#
# python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p OvmfPkg/AmdSev/AmdSevX64.dsc --quiet
#
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p OvmfPkg/OvmfPkgIa32.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -a X64 -b NOOPT -p OvmfPkg/OvmfPkgIa32X64.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p OvmfPkg/OvmfPkgX64.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p OvmfPkg/OvmfXen.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p OvmfPkg/Bhyve/BhyveX64.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p OvmfPkg/Microvm/MicrovmX64.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p PcAtChipsetPkg/PcAtChipsetPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p PcAtChipsetPkg/PcAtChipsetPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p RedfishPkg/RedfishPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p RedfishPkg/RedfishPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a ARM -b NOOPT -p RedfishPkg/RedfishPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a AARCH64 -b NOOPT -p RedfishPkg/RedfishPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a RISCV64 -b NOOPT -p RedfishPkg/RedfishPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p SecurityPkg/SecurityPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p SecurityPkg/SecurityPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a ARM -b NOOPT -p SecurityPkg/SecurityPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a AARCH64 -b NOOPT -p SecurityPkg/SecurityPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a RISCV64 -b NOOPT -p SecurityPkg/SecurityPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p ShellPkg/ShellPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p ShellPkg/ShellPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a ARM -b NOOPT -p ShellPkg/ShellPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a AARCH64 -b NOOPT -p ShellPkg/ShellPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a RISCV64 -b NOOPT -p ShellPkg/ShellPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a AARCH64 -b NOOPT -p SignedCapsulePkg/SignedCapsulePkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a ARM -b NOOPT -p SignedCapsulePkg/SignedCapsulePkg.dsc --quiet    
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p SignedCapsulePkg/SignedCapsulePkg.dsc --quiet   
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p SignedCapsulePkg/SignedCapsulePkg.dsc --quiet    
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p UefiCpuPkg/Test/UefiCpuPkgHostTest.dsc --quiet  
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p SourceLevelDebugPkg/SourceLevelDebugPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p SourceLevelDebugPkg/SourceLevelDebugPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p UefiCpuPkg/UefiCpuPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p UefiCpuPkg/UefiCpuPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p UefiCpuPkg/Test/UefiCpuPkgHostTest.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -a X64 -b NOOPT -p UefiPayloadPkg/UefiPayloadPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p UnitTestFrameworkPkg/UnitTestFrameworkPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p UnitTestFrameworkPkg/UnitTestFrameworkPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a ARM -b NOOPT -p UnitTestFrameworkPkg/UnitTestFrameworkPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a AARCH64 -b NOOPT -p UnitTestFrameworkPkg/UnitTestFrameworkPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a RISCV64 -b NOOPT -p UnitTestFrameworkPkg/UnitTestFrameworkPkg.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a IA32 -b NOOPT -p UnitTestFrameworkPkg/Test/UnitTestFrameworkPkgHostTest.dsc --quiet
python3 ../CompareBuild.py --ref1 MdkTest_uncrustify_poc_5_before --ref2  MdkTest_uncrustify_poc_5_after2 build -n 4 -t GCC5 -j Build/Build.log -a X64 -b NOOPT -p UnitTestFrameworkPkg/Test/UnitTestFrameworkPkgHostTest.dsc --quiet