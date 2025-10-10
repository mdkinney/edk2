## @file
#  Cryptographic Library Package for UEFI Security Implementation.
#  DSC include file with Feature Flag PCD settings.
#
#  Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[PcdsFeatureFlag]
#
# Use Openssl NASM assembly source files that assume Windows calling convention
# if the family is MSFT or the toolchain is CLANGPDB for Windows or Linux
# or the toolchain is MINGW32 for Windows. Otherwise, the Openssl .S assembly
# source files that assume the Linux calling convention.
#
!if "$(FAMILY)" == "MSFT" || "$(TOOL_CHAIN_TAG)" == "CLANGPDB" || $(WIN_MINGW32_BUILD)
gEfiCryptoPkgTokenSpaceGuid.PcdOpensslLibAssemblySourceStyleNasm|TRUE
!endif
