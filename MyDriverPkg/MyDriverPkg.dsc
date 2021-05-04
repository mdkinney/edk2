## @file
#  MyDriverPkg Package DSC File
#
# Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME                  = MyDriverPkg
  PLATFORM_GUID                  = ee347b1e-ac75-11eb-82a9-54e1ad3bf134
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/MyDriverPkg
  SUPPORTED_ARCHITECTURES        = IA32|IPF|X64|EBC|ARM
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses]
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiUsbLib|MdePkg/Library/UefiUsbLib/UefiUsbLib.inf
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  PostCodeLib|MdePkg/Library/BasePostCodeLibPort80/BasePostCodeLibPort80.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf

[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x0f
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x1f
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0xFFFFFFFF

  #
  # ESRT GUID for Firmware Management Protocol instances
  # Same value as FILE_GUID in MyUefiPciDriver.inf
  #
  gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceImageTypeIdGuid|{GUID(fa627640-ac75-11eb-8afc-54e1ad3bf134)}
  #
  # Unicode name string that is used to populate FMP Image Descriptor for this capsule update module
  #
  gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceImageIdName|L"MyUefiPciDriver"
  #
  # Certificates used to authenticate capsule update image
  #
  !include BaseTools/Source/Python/Pkcs7Sign/TestRoot.cer.gFmpDevicePkgTokenSpaceGuid.PcdFmpDevicePkcs7CertBufferXdr.inc
  #
  # Disable test key checking
  #
  gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceTestKeySha256Digest|{0}
  #
  # Disable FMP Variable Locking
  #
  gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceLockEventGuid|{0}

[Components]
  MyDriverPkg/MyUefiPciDriver/MyUefiPciDriver.inf {
    <LibraryClasses>
      NULL|FmpDevicePkg/FmpDxe/FmpDxeLib.inf
      RngLib|MdePkg/Library/BaseRngLibNull/BaseRngLibNull.inf
      IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
      FmpAuthenticationLib|SecurityPkg/Library/FmpAuthenticationLibPkcs7/FmpAuthenticationLibPkcs7.inf
      FmpPayloadHeaderLib|FmpDevicePkg/Library/FmpPayloadHeaderLibV1/FmpPayloadHeaderLibV1.inf
      FmpDependencyLib|FmpDevicePkg/Library/FmpDependencyLib/FmpDependencyLib.inf
      FmpDependencyCheckLib|FmpDevicePkg/Library/FmpDependencyCheckLibNull/FmpDependencyCheckLibNull.inf
      FmpDependencyDeviceLib|FmpDevicePkg/Library/FmpDependencyDeviceLibNull/FmpDependencyDeviceLibNull.inf
      #
      # Get platform capsule update policy from platform protocol if it is available.
      #
      CapsuleUpdatePolicyLib|MyDriverPkg/Library/MyCapsuleUpdatePolicyLib/MyCapsuleUpdatePolicyLib.inf

      #
      # Device specific FmpDeviceLib iplementation
      #
      FmpDeviceLib|MyDriverPkg/Library/MyFmpDeviceLib/MyFmpDeviceLib.inf
  }
