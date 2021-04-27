## @file
# EFI/PI Reference Module Package for All Architectures
#
# (C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
# Copyright (c) 2007 - 2021, Intel Corporation. All rights reserved.<BR>
# Copyright (c) Microsoft Corporation.
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME                  = MdeModule
  PLATFORM_GUID                  = 587CE499-6CBE-43cd-94E2-186218569478
  PLATFORM_VERSION               = 0.98
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/MdeModule
  SUPPORTED_ARCHITECTURES        = IA32|X64|EBC|ARM|AARCH64|RISCV64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses]
  #
  # Entry point
  #
  PeiCoreEntryPoint|MdePkg/Library/PeiCoreEntryPoint/PeiCoreEntryPoint.inf
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  DxeCoreEntryPoint|MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  #
  # Basic
  #
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciSegmentLib|MdePkg/Library/BasePciSegmentLibPci/BasePciSegmentLibPci.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  SortLib|MdeModulePkg/Library/BaseSortLib/BaseSortLib.inf
  #
  # UEFI & PI
  #
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLib.inf
  #
  # Generic Modules
  #
  UefiUsbLib|MdePkg/Library/UefiUsbLib/UefiUsbLib.inf
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  SecurityManagementLib|MdeModulePkg/Library/DxeSecurityManagementLib/DxeSecurityManagementLib.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  CustomizedDisplayLib|MdeModulePkg/Library/CustomizedDisplayLib/CustomizedDisplayLib.inf
  FrameBufferBltLib|MdeModulePkg/Library/FrameBufferBltLib/FrameBufferBltLib.inf
  #
  # Misc
  #
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
  PlatformHookLib|MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf
  ResetSystemLib|MdeModulePkg/Library/BaseResetSystemLibNull/BaseResetSystemLibNull.inf
  SmbusLib|MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  S3BootScriptLib|MdeModulePkg/Library/PiDxeS3BootScriptLib/DxeS3BootScriptLib.inf
  CpuExceptionHandlerLib|MdeModulePkg/Library/CpuExceptionHandlerLibNull/CpuExceptionHandlerLibNull.inf
  PlatformBootManagerLib|MdeModulePkg/Library/PlatformBootManagerLibNull/PlatformBootManagerLibNull.inf
  PciHostBridgeLib|MdeModulePkg/Library/PciHostBridgeLibNull/PciHostBridgeLibNull.inf
  TpmMeasurementLib|MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
  AuthVariableLib|MdeModulePkg/Library/AuthVariableLibNull/AuthVariableLibNull.inf
  VarCheckLib|MdeModulePkg/Library/VarCheckLib/VarCheckLib.inf
  FileExplorerLib|MdeModulePkg/Library/FileExplorerLib/FileExplorerLib.inf
  NonDiscoverableDeviceRegistrationLib|MdeModulePkg/Library/NonDiscoverableDeviceRegistrationLib/NonDiscoverableDeviceRegistrationLib.inf

  FmpAuthenticationLib|MdeModulePkg/Library/FmpAuthenticationLibNull/FmpAuthenticationLibNull.inf
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf
  BmpSupportLib|MdeModulePkg/Library/BaseBmpSupportLib/BaseBmpSupportLib.inf
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf
  DisplayUpdateProgressLib|MdeModulePkg/Library/DisplayUpdateProgressLibGraphics/DisplayUpdateProgressLibGraphics.inf
  VariablePolicyHelperLib|MdeModulePkg/Library/VariablePolicyHelperLib/VariablePolicyHelperLib.inf
  MmUnblockMemoryLib|MdePkg/Library/MmUnblockMemoryLib/MmUnblockMemoryLibNull.inf

[LibraryClasses.EBC.PEIM]
  IoLib|MdePkg/Library/PeiIoLibCpuIo/PeiIoLibCpuIo.inf

[LibraryClasses.common.PEI_CORE]
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf

[LibraryClasses.common.PEIM]
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxPeiLib.inf

[LibraryClasses.common.DXE_CORE]
  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  MemoryAllocationLib|MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf

[LibraryClasses.common.DXE_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxDxeLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibFmp/DxeCapsuleLib.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxDxeLib.inf
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibFmp/DxeRuntimeCapsuleLib.inf
  VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLibRuntimeDxe.inf

[LibraryClasses.common.SMM_CORE]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdeModulePkg/Library/PiSmmCoreMemoryAllocationLib/PiSmmCoreMemoryAllocationLib.inf
  SmmServicesTableLib|MdeModulePkg/Library/PiSmmCoreSmmServicesTableLib/PiSmmCoreSmmServicesTableLib.inf
  SmmCorePlatformHookLib|MdeModulePkg/Library/SmmCorePlatformHookLibNull/SmmCorePlatformHookLibNull.inf
  SmmMemLib|MdePkg/Library/SmmMemLib/SmmMemLib.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  MemoryAllocationLib|MdePkg/Library/SmmMemoryAllocationLib/SmmMemoryAllocationLib.inf
  MmServicesTableLib|MdePkg/Library/MmServicesTableLib/MmServicesTableLib.inf
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxSmmLib.inf
  SmmMemLib|MdePkg/Library/SmmMemLib/SmmMemLib.inf

[LibraryClasses.common.UEFI_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxDxeLib.inf

[LibraryClasses.common.UEFI_APPLICATION]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibStdErr/UefiDebugLibStdErr.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf

[LibraryClasses.common.MM_STANDALONE]
  HobLib|MdeModulePkg/Library/BaseHobLibNull/BaseHobLibNull.inf
  MemoryAllocationLib|MdeModulePkg/Library/BaseMemoryAllocationLibNull/BaseMemoryAllocationLibNull.inf
  StandaloneMmDriverEntryPoint|MdePkg/Library/StandaloneMmDriverEntryPoint/StandaloneMmDriverEntryPoint.inf
  MmServicesTableLib|MdePkg/Library/StandaloneMmServicesTableLib/StandaloneMmServicesTableLib.inf
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxStandaloneMmLib.inf
  MemLib|StandaloneMmPkg/Library/StandaloneMmMemLib/StandaloneMmMemLib.inf

[LibraryClasses.ARM, LibraryClasses.AARCH64]
  ArmLib|ArmPkg/Library/ArmLib/ArmBaseLib.inf
  ArmMmuLib|ArmPkg/Library/ArmMmuLib/ArmMmuBaseLib.inf
  LockBoxLib|MdeModulePkg/Library/LockBoxNullLib/LockBoxNullLib.inf

  #
  # It is not possible to prevent ARM compiler calls to generic intrinsic functions.
  # This library provides the instrinsic functions generated by a given compiler.
  # [LibraryClasses.ARM] and NULL mean link this library into all ARM images.
  #
  NULL|ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf

  #
  # Since software stack checking may be heuristically enabled by the compiler
  # include BaseStackCheckLib unconditionally.
  #
  NULL|MdePkg/Library/BaseStackCheckLib/BaseStackCheckLib.inf

[LibraryClasses.EBC, LibraryClasses.RISCV64]
  LockBoxLib|MdeModulePkg/Library/LockBoxNullLib/LockBoxNullLib.inf

[PcdsFeatureFlag]
  gEfiMdePkgTokenSpaceGuid.PcdDriverDiagnostics2Disable|TRUE
  gEfiMdePkgTokenSpaceGuid.PcdComponentName2Disable|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdInstallAcpiSdtProtocol|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDevicePathSupportDevicePathFromText|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDevicePathSupportDevicePathToText|FALSE

[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x0f
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x06
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxSizeNonPopulateCapsule|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxSizePopulateCapsule|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxPeiPerformanceLogEntries|28

[PcdsDynamicExDefault]
  gEfiMdeModulePkgTokenSpaceGuid.PcdRecoveryFileName|L"FVMAIN.FV"

[Components]
  MdeModulePkg/Application/HelloWorld/HelloWorld.inf
  MdeModulePkg/Application/DumpDynPcd/DumpDynPcd.inf
  MdeModulePkg/Application/MemoryProfileInfo/MemoryProfileInfo.inf

  MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
  MdeModulePkg/Logo/Logo.inf
  MdeModulePkg/Logo/LogoDxe.inf
  MdeModulePkg/Library/BaseSortLib/BaseSortLib.inf
  MdeModulePkg/Library/BootMaintenanceManagerUiLib/BootMaintenanceManagerUiLib.inf
  MdeModulePkg/Library/BootManagerUiLib/BootManagerUiLib.inf
  MdeModulePkg/Library/CustomizedDisplayLib/CustomizedDisplayLib.inf
  MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
  MdeModulePkg/Library/DeviceManagerUiLib/DeviceManagerUiLib.inf
  MdeModulePkg/Library/LockBoxNullLib/LockBoxNullLib.inf
  MdeModulePkg/Library/PciHostBridgeLibNull/PciHostBridgeLibNull.inf
  MdeModulePkg/Library/PiSmmCoreSmmServicesTableLib/PiSmmCoreSmmServicesTableLib.inf
  MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  MdeModulePkg/Library/BaseHobLibNull/BaseHobLibNull.inf
  MdeModulePkg/Library/BaseMemoryAllocationLibNull/BaseMemoryAllocationLibNull.inf
  MdeModulePkg/Library/VariablePolicyHelperLib/VariablePolicyHelperLib.inf

  MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciHostBridgeDxe.inf
  MdeModulePkg/Bus/Pci/PciSioSerialDxe/PciSioSerialDxe.inf
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf
  MdeModulePkg/Bus/Pci/IncompatiblePciDeviceSupportDxe/IncompatiblePciDeviceSupportDxe.inf
  MdeModulePkg/Bus/Pci/NvmExpressDxe/NvmExpressDxe.inf
  MdeModulePkg/Bus/Pci/NvmExpressPei/NvmExpressPei.inf
  MdeModulePkg/Bus/Pci/SdMmcPciHcDxe/SdMmcPciHcDxe.inf
  MdeModulePkg/Bus/Pci/SdMmcPciHcPei/SdMmcPciHcPei.inf
  MdeModulePkg/Bus/Sd/EmmcBlockIoPei/EmmcBlockIoPei.inf
  MdeModulePkg/Bus/Sd/SdBlockIoPei/SdBlockIoPei.inf
  MdeModulePkg/Bus/Sd/EmmcDxe/EmmcDxe.inf
  MdeModulePkg/Bus/Sd/SdDxe/SdDxe.inf
  MdeModulePkg/Bus/Pci/UfsPciHcDxe/UfsPciHcDxe.inf
  MdeModulePkg/Bus/Ufs/UfsPassThruDxe/UfsPassThruDxe.inf
  MdeModulePkg/Bus/Pci/UfsPciHcPei/UfsPciHcPei.inf
  MdeModulePkg/Bus/Ufs/UfsBlockIoPei/UfsBlockIoPei.inf
  MdeModulePkg/Bus/Pci/XhciDxe/XhciDxe.inf
  MdeModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf
  MdeModulePkg/Bus/Pci/UhciDxe/UhciDxe.inf
  MdeModulePkg/Bus/Pci/UhciPei/UhciPei.inf
  MdeModulePkg/Bus/Pci/EhciPei/EhciPei.inf
  MdeModulePkg/Bus/Pci/XhciPei/XhciPei.inf
  MdeModulePkg/Bus/Pci/IdeBusPei/IdeBusPei.inf
  MdeModulePkg/Bus/Usb/UsbBusPei/UsbBusPei.inf
  MdeModulePkg/Bus/Usb/UsbBotPei/UsbBotPei.inf
  MdeModulePkg/Bus/Pci/SataControllerDxe/SataControllerDxe.inf
  MdeModulePkg/Bus/Ata/AtaBusDxe/AtaBusDxe.inf
  MdeModulePkg/Bus/Ata/AtaAtapiPassThru/AtaAtapiPassThru.inf
  MdeModulePkg/Bus/Ata/AhciPei/AhciPei.inf
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf
  MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf
  MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf
  MdeModulePkg/Bus/Usb/UsbMouseAbsolutePointerDxe/UsbMouseAbsolutePointerDxe.inf
  MdeModulePkg/Bus/Usb/UsbMouseDxe/UsbMouseDxe.inf
  MdeModulePkg/Bus/I2c/I2cDxe/I2cBusDxe.inf
  MdeModulePkg/Bus/I2c/I2cDxe/I2cHostDxe.inf
  MdeModulePkg/Bus/I2c/I2cDxe/I2cDxe.inf
  MdeModulePkg/Bus/Isa/IsaBusDxe/IsaBusDxe.inf
  MdeModulePkg/Bus/Isa/Ps2KeyboardDxe/Ps2KeyboardDxe.inf
  MdeModulePkg/Bus/Isa/Ps2MouseDxe/Ps2MouseDxe.inf
  MdeModulePkg/Bus/Pci/NonDiscoverablePciDeviceDxe/NonDiscoverablePciDeviceDxe.inf

  MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf
  MdeModulePkg/Core/Pei/PeiMain.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf

  MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf
  MdeModulePkg/Library/UefiMemoryAllocationProfileLib/UefiMemoryAllocationProfileLib.inf
  MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf
  MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationProfileLib.inf
  MdeModulePkg/Library/DxeCorePerformanceLib/DxeCorePerformanceLib.inf
  MdeModulePkg/Library/DxeCrc32GuidedSectionExtractLib/DxeCrc32GuidedSectionExtractLib.inf
  MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
  MdeModulePkg/Library/DxeResetSystemLib/DxeResetSystemLib.inf
  MdeModulePkg/Library/DxePrintLibPrint2Protocol/DxePrintLibPrint2Protocol.inf
  MdeModulePkg/Library/PeiCrc32GuidedSectionExtractLib/PeiCrc32GuidedSectionExtractLib.inf
  MdeModulePkg/Library/PeiPerformanceLib/PeiPerformanceLib.inf
  MdeModulePkg/Library/PeiResetSystemLib/PeiResetSystemLib.inf
  MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  MdeModulePkg/Library/ResetUtilityLib/ResetUtilityLib.inf
  MdeModulePkg/Library/BaseResetSystemLibNull/BaseResetSystemLibNull.inf
  MdeModulePkg/Library/DxeSecurityManagementLib/DxeSecurityManagementLib.inf
  MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  MdeModulePkg/Library/RuntimeDxeReportStatusCodeLib/RuntimeDxeReportStatusCodeLib.inf
  MdeModulePkg/Library/RuntimeResetSystemLib/RuntimeResetSystemLib.inf
  MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
  MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf
  MdeModulePkg/Library/DxeDebugPrintErrorLevelLib/DxeDebugPrintErrorLevelLib.inf
  MdeModulePkg/Library/PiDxeS3BootScriptLib/DxeS3BootScriptLib.inf
  MdeModulePkg/Library/PeiDebugPrintHobLib/PeiDebugPrintHobLib.inf
  MdeModulePkg/Library/CpuExceptionHandlerLibNull/CpuExceptionHandlerLibNull.inf
  MdeModulePkg/Library/PlatformHookLibSerialPortPpi/PlatformHookLibSerialPortPpi.inf
  MdeModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  MdeModulePkg/Library/PeiDebugLibDebugPpi/PeiDebugLibDebugPpi.inf
  MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  MdeModulePkg/Library/PlatformBootManagerLibNull/PlatformBootManagerLibNull.inf
  MdeModulePkg/Library/BootLogoLib/BootLogoLib.inf
  MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
  MdeModulePkg/Library/AuthVariableLibNull/AuthVariableLibNull.inf
  MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLib.inf
  MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLibRuntimeDxe.inf
  MdeModulePkg/Library/VarCheckPolicyLib/VarCheckPolicyLib.inf
  MdeModulePkg/Library/VarCheckPolicyLib/VarCheckPolicyLibStandaloneMm.inf
  MdeModulePkg/Library/VarCheckLib/VarCheckLib.inf
  MdeModulePkg/Library/VarCheckHiiLib/VarCheckHiiLib.inf
  MdeModulePkg/Library/VarCheckPcdLib/VarCheckPcdLib.inf
  MdeModulePkg/Library/PlatformVarCleanupLib/PlatformVarCleanupLib.inf
  MdeModulePkg/Library/FileExplorerLib/FileExplorerLib.inf
  MdeModulePkg/Library/DxeFileExplorerProtocol/DxeFileExplorerProtocol.inf
  MdeModulePkg/Library/BaseIpmiLibNull/BaseIpmiLibNull.inf
  MdeModulePkg/Library/DxeIpmiLibIpmiProtocol/DxeIpmiLibIpmiProtocol.inf
  MdeModulePkg/Library/PeiIpmiLibIpmiPpi/PeiIpmiLibIpmiPpi.inf
  MdeModulePkg/Library/SmmIpmiLibSmmIpmiProtocol/SmmIpmiLibSmmIpmiProtocol.inf
  MdeModulePkg/Library/FrameBufferBltLib/FrameBufferBltLib.inf
  MdeModulePkg/Library/NonDiscoverableDeviceRegistrationLib/NonDiscoverableDeviceRegistrationLib.inf
  MdeModulePkg/Library/BaseBmpSupportLib/BaseBmpSupportLib.inf
  MdeModulePkg/Library/DisplayUpdateProgressLibGraphics/DisplayUpdateProgressLibGraphics.inf
  MdeModulePkg/Library/DisplayUpdateProgressLibText/DisplayUpdateProgressLibText.inf

  MdeModulePkg/Universal/BdsDxe/BdsDxe.inf
  MdeModulePkg/Application/BootManagerMenuApp/BootManagerMenuApp.inf
  MdeModulePkg/Application/UiApp/UiApp.inf{
    <LibraryClasses>
      NULL|MdeModulePkg/Library/DeviceManagerUiLib/DeviceManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootManagerUiLib/BootManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootMaintenanceManagerUiLib/BootMaintenanceManagerUiLib.inf
  }
  MdeModulePkg/Universal/DriverHealthManagerDxe/DriverHealthManagerDxe.inf
  MdeModulePkg/Universal/BootManagerPolicyDxe/BootManagerPolicyDxe.inf
  MdeModulePkg/Universal/CapsulePei/CapsulePei.inf
  MdeModulePkg/Universal/CapsuleOnDiskLoadPei/CapsuleOnDiskLoadPei.inf
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  MdeModulePkg/Universal/Console/GraphicsOutputDxe/GraphicsOutputDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  MdeModulePkg/Universal/DebugPortDxe/DebugPortDxe.inf
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  MdeModulePkg/Universal/PrintDxe/PrintDxe.inf
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  MdeModulePkg/Universal/Disk/UdfDxe/UdfDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  MdeModulePkg/Universal/Disk/CdExpressPei/CdExpressPei.inf
  MdeModulePkg/Universal/DriverSampleDxe/DriverSampleDxe.inf
  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  MdeModulePkg/Universal/MemoryTest/GenericMemoryTestDxe/GenericMemoryTestDxe.inf
  MdeModulePkg/Universal/MemoryTest/NullMemoryTestDxe/NullMemoryTestDxe.inf
  MdeModulePkg/Universal/Metronome/Metronome.inf
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  MdeModulePkg/Universal/ResetSystemPei/ResetSystemPei.inf {
    <LibraryClasses>
      ResetSystemLib|MdeModulePkg/Library/BaseResetSystemLibNull/BaseResetSystemLibNull.inf
  }
  MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf {
    <LibraryClasses>
      ResetSystemLib|MdeModulePkg/Library/BaseResetSystemLibNull/BaseResetSystemLibNull.inf
  }
  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf
  MdeModulePkg/Universal/SmbiosMeasurementDxe/SmbiosMeasurementDxe.inf

  MdeModulePkg/Universal/PcatSingleSegmentPciCfg2Pei/PcatSingleSegmentPciCfg2Pei.inf
  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf
  MdeModulePkg/Universal/PCD/Pei/Pcd.inf
  MdeModulePkg/Universal/PlatformDriOverrideDxe/PlatformDriOverrideDxe.inf

  MdeModulePkg/Universal/ReportStatusCodeRouter/Pei/ReportStatusCodeRouterPei.inf
  MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf

  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf
  MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf
  MdeModulePkg/Universal/DisplayEngineDxe/DisplayEngineDxe.inf
  MdeModulePkg/Application/VariableInfo/VariableInfo.inf
  MdeModulePkg/Universal/FaultTolerantWritePei/FaultTolerantWritePei.inf
  MdeModulePkg/Universal/Variable/Pei/VariablePei.inf
  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  MdeModulePkg/Universal/TimestampDxe/TimestampDxe.inf
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf

  MdeModulePkg/Universal/Acpi/AcpiPlatformDxe/AcpiPlatformDxe.inf
  MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe.inf
  MdeModulePkg/Universal/HiiResourcesSampleDxe/HiiResourcesSampleDxe.inf
  MdeModulePkg/Universal/LegacyRegion2Dxe/LegacyRegion2Dxe.inf

  MdeModulePkg/Universal/StatusCodeHandler/Pei/StatusCodeHandlerPei.inf
  MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf

  MdeModulePkg/Universal/Acpi/FirmwarePerformanceDataTablePei/FirmwarePerformancePei.inf {
    <LibraryClasses>
      LockBoxLib|MdeModulePkg/Library/LockBoxNullLib/LockBoxNullLib.inf
  }
  MdeModulePkg/Universal/Acpi/FirmwarePerformanceDataTableDxe/FirmwarePerformanceDxe.inf
  MdeModulePkg/Universal/Acpi/BootGraphicsResourceTableDxe/BootGraphicsResourceTableDxe.inf
  MdeModulePkg/Universal/SectionExtractionDxe/SectionExtractionDxe.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/DxeCrc32GuidedSectionExtractLib/DxeCrc32GuidedSectionExtractLib.inf
  }
  MdeModulePkg/Universal/SectionExtractionPei/SectionExtractionPei.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/PeiCrc32GuidedSectionExtractLib/PeiCrc32GuidedSectionExtractLib.inf
  }

  MdeModulePkg/Universal/FvSimpleFileSystemDxe/FvSimpleFileSystemDxe.inf
  MdeModulePkg/Universal/EsrtDxe/EsrtDxe.inf
  MdeModulePkg/Universal/EsrtFmpDxe/EsrtFmpDxe.inf

  MdeModulePkg/Universal/FileExplorerDxe/FileExplorerDxe.inf  {
    <LibraryClasses>
      FileExplorerLib|MdeModulePkg/Library/FileExplorerLib/FileExplorerLib.inf
  }

  MdeModulePkg/Universal/SerialDxe/SerialDxe.inf
  MdeModulePkg/Universal/LoadFileOnFv2/LoadFileOnFv2.inf

  MdeModulePkg/Universal/DebugServicePei/DebugServicePei.inf

  MdeModulePkg/Application/CapsuleApp/CapsuleApp.inf
  MdeModulePkg/Library/FmpAuthenticationLibNull/FmpAuthenticationLibNull.inf
  MdeModulePkg/Library/DxeCapsuleLibFmp/DxeCapsuleLib.inf
  MdeModulePkg/Library/DxeCapsuleLibFmp/DxeRuntimeCapsuleLib.inf

[Components.IA32, Components.X64, Components.AARCH64]
  MdeModulePkg/Universal/EbcDxe/EbcDxe.inf
  MdeModulePkg/Universal/EbcDxe/EbcDebugger.inf
  MdeModulePkg/Universal/EbcDxe/EbcDebuggerConfig.inf

[Components.IA32, Components.X64, Components.ARM, Components.AARCH64]
  MdeModulePkg/Library/BrotliCustomDecompressLib/BrotliCustomDecompressLib.inf
  MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
  MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/DxeCrc32GuidedSectionExtractLib/DxeCrc32GuidedSectionExtractLib.inf
  }

!if $(TOOL_CHAIN_TAG) != "XCODE5"
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteStandaloneMm.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableStandaloneMm.inf
!endif

[Components.IA32, Components.X64]
  MdeModulePkg/Universal/DebugSupportDxe/DebugSupportDxe.inf
  MdeModulePkg/Application/SmiHandlerProfileInfo/SmiHandlerProfileInfo.inf
  MdeModulePkg/Core/PiSmmCore/PiSmmIpl.inf
  MdeModulePkg/Core/PiSmmCore/PiSmmCore.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableSmm.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/VarCheckPolicyLib/VarCheckPolicyLib.inf
      NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
      NULL|MdeModulePkg/Library/VarCheckHiiLib/VarCheckHiiLib.inf
      NULL|MdeModulePkg/Library/VarCheckPcdLib/VarCheckPcdLib.inf
  }
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
      NULL|MdeModulePkg/Library/VarCheckHiiLib/VarCheckHiiLib.inf
      NULL|MdeModulePkg/Library/VarCheckPcdLib/VarCheckPcdLib.inf
  }
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableSmmRuntimeDxe.inf
  MdeModulePkg/Library/SmmReportStatusCodeLib/SmmReportStatusCodeLib.inf
  MdeModulePkg/Library/SmmReportStatusCodeLib/StandaloneMmReportStatusCodeLib.inf
  MdeModulePkg/Universal/StatusCodeHandler/Smm/StatusCodeHandlerSmm.inf
  MdeModulePkg/Universal/StatusCodeHandler/Smm/StatusCodeHandlerStandaloneMm.inf
  MdeModulePkg/Universal/ReportStatusCodeRouter/Smm/ReportStatusCodeRouterSmm.inf
  MdeModulePkg/Universal/ReportStatusCodeRouter/Smm/ReportStatusCodeRouterStandaloneMm.inf
  MdeModulePkg/Universal/LockBox/SmmLockBox/SmmLockBox.inf
  MdeModulePkg/Library/SmmMemoryAllocationProfileLib/SmmMemoryAllocationProfileLib.inf
  MdeModulePkg/Library/PiSmmCoreMemoryAllocationLib/PiSmmCoreMemoryAllocationProfileLib.inf
  MdeModulePkg/Library/PiSmmCoreMemoryAllocationLib/PiSmmCoreMemoryAllocationLib.inf
  MdeModulePkg/Library/SmmCorePerformanceLib/SmmCorePerformanceLib.inf
  MdeModulePkg/Library/SmmPerformanceLib/SmmPerformanceLib.inf
  MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxPeiLib.inf
  MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxDxeLib.inf
  MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxSmmLib.inf
  MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxStandaloneMmLib.inf
  MdeModulePkg/Library/SmmCorePlatformHookLibNull/SmmCorePlatformHookLibNull.inf
  MdeModulePkg/Library/SmmSmiHandlerProfileLib/SmmSmiHandlerProfileLib.inf
  MdeModulePkg/Library/SmmSmiHandlerProfileLib/StandaloneMmSmiHandlerProfileLib.inf
  MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaArchCustomDecompressLib.inf
  MdeModulePkg/Universal/Acpi/BootScriptExecutorDxe/BootScriptExecutorDxe.inf
  MdeModulePkg/Universal/Acpi/S3SaveStateDxe/S3SaveStateDxe.inf
  MdeModulePkg/Universal/Acpi/SmmS3SaveState/SmmS3SaveState.inf
  MdeModulePkg/Universal/Acpi/FirmwarePerformanceDataTableSmm/FirmwarePerformanceSmm.inf
  MdeModulePkg/Universal/Acpi/FirmwarePerformanceDataTableSmm/FirmwarePerformanceStandaloneMm.inf
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteSmm.inf
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteSmmDxe.inf
#  MdeModulePkg/Universal/RegularExpressionDxe/RegularExpressionDxe.inf
  MdeModulePkg/Universal/SmmCommunicationBufferDxe/SmmCommunicationBufferDxe.inf
  MdeModulePkg/Universal/Disk/RamDiskDxe/RamDiskDxe.inf

[Components.X64]
  MdeModulePkg/Universal/CapsulePei/CapsuleX64.inf

[BuildOptions]

[Defines]
  DEFINE RC_PATH    = C:\Program Files (x86)\Windows Kits\10\bin\10.0.18362.0\x86\rc.exe

  DEFINE VS2019_PREFIX = C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Tools\MSVC\14.27.29110\
  DEFINE VS_HOST            = x86
  DEFINE VS2019_BIN         = $(VS2019_PREFIX)bin
  DEFINE VS2019_BIN_HOST    = $(VS2019_BIN)\Host$(VS_HOST)\$(VS_HOST)
  DEFINE VS2019_BIN_IA32    = $(VS2019_BIN)\Host$(VS_HOST)\x86
  DEFINE VS2019_BIN_X64     = $(VS2019_BIN)\Host$(VS_HOST)\x64
  DEFINE VS2019_BIN_ARM     = $(VS2019_BIN)\Host$(VS_HOST)\arm
  DEFINE VS2019_BIN_AARCH64 = $(VS2019_BIN)\Host$(VS_HOST)\arm64
  
  DEFINE MSFT_DEPS_FLAGS         = /showIncludes

  DEFINE EBC_BIN          = C:\Program Files\Intel\EBC\Bin
  DEFINE EBC_BINX86       = C:\Program Files (x86)\Intel\EBC\Bin

  DEFINE IASL_FLAGS              =
  DEFINE IASL_OUTFLAGS           = -p

  DEFINE WIN_IASL_BIN            = C:\ASL\iasl.exe
  DEFINE DEFAULT_WIN_ASL_FLAGS    = $(IASL_FLAGS)
  DEFINE DEFAULT_WIN_ASL_OUTFLAGS = $(IASL_OUTFLAGS)
  DEFINE MSFT_ASLPP_FLAGS        = /nologo /E /C /FIAutoGen.h
  DEFINE MSFT_ASLCC_FLAGS        = /nologo /c /FIAutoGen.h /TC /Dmain=ReferenceAcpiTable
  DEFINE MSFT_ASLDLINK_FLAGS     = /NODEFAULTLIB /ENTRY:ReferenceAcpiTable /SUBSYSTEM:CONSOLE


[BuildOptions]
*_MYTOOLCHAIN_*_*_FAMILY        == MSFT
#*_MYTOOLCHAIN_*_*_DLL           == $(VS2019_BIN_HOST)

*_MYTOOLCHAIN_*_MAKE_PATH       == $(VS2019_BIN_HOST)\nmake.exe
*_MYTOOLCHAIN_*_MAKE_FLAGS      == /nologo
*_MYTOOLCHAIN_*_RC_PATH         == $(RC_PATH)

*_MYTOOLCHAIN_*_SLINK_FLAGS     == /NOLOGO /LTCG
*_MYTOOLCHAIN_*_APP_FLAGS       == /nologo /E /TC
*_MYTOOLCHAIN_*_PP_FLAGS        == /nologo /E /TC /FIAutoGen.h
*_MYTOOLCHAIN_*_VFRPP_FLAGS     == /nologo /E /TC /DVFRCOMPILE /FI$(MODULE_NAME)StrDefs.h
*_MYTOOLCHAIN_*_DLINK2_FLAGS    == /WHOLEARCHIVE
*_MYTOOLCHAIN_*_ASM16_PATH      == $(VS2019_BIN_IA32)\ml.exe
*_MYTOOLCHAIN_*_DEPS_FLAGS      == $(MSFT_DEPS_FLAGS)
##################
# ASL definitions
##################
*_MYTOOLCHAIN_*_ASL_PATH        == $(WIN_IASL_BIN)
*_MYTOOLCHAIN_*_ASL_FLAGS       == $(DEFAULT_WIN_ASL_FLAGS)
*_MYTOOLCHAIN_*_ASL_OUTFLAGS    == $(DEFAULT_WIN_ASL_OUTFLAGS)
*_MYTOOLCHAIN_*_ASLCC_FLAGS     == $(MSFT_ASLCC_FLAGS)
*_MYTOOLCHAIN_*_ASLPP_FLAGS     == $(MSFT_ASLPP_FLAGS)
*_MYTOOLCHAIN_*_ASLDLINK_FLAGS  == $(MSFT_ASLDLINK_FLAGS)

##################
# IA32 definitions
##################
*_MYTOOLCHAIN_IA32_CC_PATH      == $(VS2019_BIN_IA32)\cl.exe
*_MYTOOLCHAIN_IA32_VFRPP_PATH   == $(VS2019_BIN_IA32)\cl.exe
*_MYTOOLCHAIN_IA32_ASLCC_PATH   == $(VS2019_BIN_IA32)\cl.exe
*_MYTOOLCHAIN_IA32_ASLPP_PATH   == $(VS2019_BIN_IA32)\cl.exe
*_MYTOOLCHAIN_IA32_SLINK_PATH   == $(VS2019_BIN_IA32)\lib.exe
*_MYTOOLCHAIN_IA32_DLINK_PATH   == $(VS2019_BIN_IA32)\link.exe
*_MYTOOLCHAIN_IA32_ASLDLINK_PATH== $(VS2019_BIN_IA32)\link.exe
*_MYTOOLCHAIN_IA32_APP_PATH     == $(VS2019_BIN_IA32)\cl.exe
*_MYTOOLCHAIN_IA32_PP_PATH      == $(VS2019_BIN_IA32)\cl.exe
*_MYTOOLCHAIN_IA32_ASM_PATH     == $(VS2019_BIN_IA32)\ml.exe

  DEBUG_MYTOOLCHAIN_IA32_CC_FLAGS    == /nologo /arch:IA32 /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1b2 /GL /FIAutoGen.h /EHs-c- /GR- /GF /Gy /Z7 /Gw
RELEASE_MYTOOLCHAIN_IA32_CC_FLAGS    == /nologo /arch:IA32 /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1b2 /GL /FIAutoGen.h /EHs-c- /GR- /GF /Gw
NOOPT_MYTOOLCHAIN_IA32_CC_FLAGS      == /nologo /arch:IA32 /c /WX /GS- /W4 /Gs32768 /D UNICODE /FIAutoGen.h /EHs-c- /GR- /GF /Gy /Z7 /Od

  DEBUG_MYTOOLCHAIN_IA32_ASM_FLAGS   == /nologo /c /WX /W3 /Cx /coff /Zd /Zi
RELEASE_MYTOOLCHAIN_IA32_ASM_FLAGS   == /nologo /c /WX /W3 /Cx /coff /Zd
NOOPT_MYTOOLCHAIN_IA32_ASM_FLAGS     == /nologo /c /WX /W3 /Cx /coff /Zd /Zi

  DEBUG_MYTOOLCHAIN_IA32_NASM_FLAGS  == -Ox -f win32 -g
RELEASE_MYTOOLCHAIN_IA32_NASM_FLAGS  == -Ox -f win32
NOOPT_MYTOOLCHAIN_IA32_NASM_FLAGS    == -O0 -f win32 -g

  DEBUG_MYTOOLCHAIN_IA32_DLINK_FLAGS == /NOLOGO /NODEFAULTLIB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:32 /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:X86 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG
RELEASE_MYTOOLCHAIN_IA32_DLINK_FLAGS == /NOLOGO /NODEFAULTLIB /IGNORE:4001 /IGNORE:4254 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:32 /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:X86 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /MERGE:.rdata==.data
NOOPT_MYTOOLCHAIN_IA32_DLINK_FLAGS   == /NOLOGO /NODEFAULTLIB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:32 /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:X86 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG

##################
# X64 definitions
##################
*_MYTOOLCHAIN_X64_CC_PATH       == $(VS2019_BIN_X64)\cl.exe
*_MYTOOLCHAIN_X64_PP_PATH       == $(VS2019_BIN_X64)\cl.exe
*_MYTOOLCHAIN_X64_APP_PATH      == $(VS2019_BIN_X64)\cl.exe
*_MYTOOLCHAIN_X64_VFRPP_PATH    == $(VS2019_BIN_X64)\cl.exe
*_MYTOOLCHAIN_X64_ASLCC_PATH    == $(VS2019_BIN_X64)\cl.exe
*_MYTOOLCHAIN_X64_ASLPP_PATH    == $(VS2019_BIN_X64)\cl.exe
*_MYTOOLCHAIN_X64_ASM_PATH      == $(VS2019_BIN_X64)\ml64.exe
*_MYTOOLCHAIN_X64_SLINK_PATH    == $(VS2019_BIN_X64)\lib.exe
*_MYTOOLCHAIN_X64_DLINK_PATH    == $(VS2019_BIN_X64)\link.exe
*_MYTOOLCHAIN_X64_ASLDLINK_PATH == $(VS2019_BIN_X64)\link.exe

  DEBUG_MYTOOLCHAIN_X64_CC_FLAGS     == /nologo /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1b2s /GL /Gy /FIAutoGen.h /EHs-c- /GR- /GF /Z7 /Gw
RELEASE_MYTOOLCHAIN_X64_CC_FLAGS     == /nologo /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1b2s /GL /Gy /FIAutoGen.h /EHs-c- /GR- /GF /Gw
NOOPT_MYTOOLCHAIN_X64_CC_FLAGS       == /nologo /c /WX /GS- /W4 /Gs32768 /D UNICODE /Gy /FIAutoGen.h /EHs-c- /GR- /GF /Z7 /Od

  DEBUG_MYTOOLCHAIN_X64_ASM_FLAGS    == /nologo /c /WX /W3 /Cx /Zd /Zi
RELEASE_MYTOOLCHAIN_X64_ASM_FLAGS    == /nologo /c /WX /W3 /Cx /Zd
NOOPT_MYTOOLCHAIN_X64_ASM_FLAGS      == /nologo /c /WX /W3 /Cx /Zd /Zi

  DEBUG_MYTOOLCHAIN_X64_NASM_FLAGS   == -Ox -f win64 -g
RELEASE_MYTOOLCHAIN_X64_NASM_FLAGS   == -Ox -f win64
NOOPT_MYTOOLCHAIN_X64_NASM_FLAGS     == -O0 -f win64 -g

  DEBUG_MYTOOLCHAIN_X64_DLINK_FLAGS  == /NOLOGO /NODEFAULTLIB /IGNORE:4001 /IGNORE:4281 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:32 /SECTION:.xdata,D /SECTION:.pdata,D /Machine:X64 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG
RELEASE_MYTOOLCHAIN_X64_DLINK_FLAGS  == /NOLOGO /NODEFAULTLIB /IGNORE:4001 /IGNORE:4281 /IGNORE:4254 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:32 /SECTION:.xdata,D /SECTION:.pdata,D /Machine:X64 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /MERGE:.rdata=.data
NOOPT_MYTOOLCHAIN_X64_DLINK_FLAGS    == /NOLOGO /NODEFAULTLIB /IGNORE:4001 /IGNORE:4281 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:32 /SECTION:.xdata,D /SECTION:.pdata,D /Machine:X64 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG

#################
# ARM definitions
#################
*_MYTOOLCHAIN_ARM_CC_PATH              == $(VS2019_BIN_ARM)\cl.exe
*_MYTOOLCHAIN_ARM_VFRPP_PATH           == $(VS2019_BIN_ARM)\cl.exe
*_MYTOOLCHAIN_ARM_SLINK_PATH           == $(VS2019_BIN_ARM)\lib.exe
*_MYTOOLCHAIN_ARM_DLINK_PATH           == $(VS2019_BIN_ARM)\link.exe
*_MYTOOLCHAIN_ARM_APP_PATH             == $(VS2019_BIN_ARM)\cl.exe
*_MYTOOLCHAIN_ARM_PP_PATH              == $(VS2019_BIN_ARM)\cl.exe
*_MYTOOLCHAIN_ARM_ASM_PATH             == $(VS2019_BIN_ARM)\armasm.exe
*_MYTOOLCHAIN_ARM_ASLCC_PATH           == $(VS2019_BIN_ARM)\cl.exe
*_MYTOOLCHAIN_ARM_ASLPP_PATH           == $(VS2019_BIN_ARM)\cl.exe
*_MYTOOLCHAIN_ARM_ASLDLINK_PATH        == $(VS2019_BIN_ARM)\link.exe

  DEBUG_MYTOOLCHAIN_ARM_CC_FLAGS       == /nologo /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1b2 /GL /FIAutoGen.h /EHs-c- /GR- /GF /Gy /Zi /Gw /Oi-
RELEASE_MYTOOLCHAIN_ARM_CC_FLAGS       == /nologo /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1b2 /GL /FIAutoGen.h /EHs-c- /GR- /GF /Gw /Oi-
NOOPT_MYTOOLCHAIN_ARM_CC_FLAGS         == /nologo /c /WX /GS- /W4 /Gs32768 /D UNICODE /FIAutoGen.h /EHs-c- /GR- /GF /Gy /Zi /Od /Oi-

  DEBUG_MYTOOLCHAIN_ARM_ASM_FLAGS      == /nologo /g
RELEASE_MYTOOLCHAIN_ARM_ASM_FLAGS      == /nologo
NOOPT_MYTOOLCHAIN_ARM_ASM_FLAGS        == /nologo

  DEBUG_MYTOOLCHAIN_ARM_DLINK_FLAGS    == /NOLOGO /NODEFAULTLIB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:ARM /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG
RELEASE_MYTOOLCHAIN_ARM_DLINK_FLAGS    == /NOLOGO /NODEFAULTLIB /IGNORE:4001 /IGNORE:4254 /OPT:REF /OPT:ICF=10 /MAP /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:ARM /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /MERGE:.rdata=.data
NOOPT_MYTOOLCHAIN_ARM_DLINK_FLAGS      == /NOLOGO /NODEFAULTLIB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:ARM /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /BASE:0 /DRIVER /DEBUG

#####################
# AARCH64 definitions
#####################
*_MYTOOLCHAIN_AARCH64_CC_PATH           == $(VS2019_BIN_AARCH64)\cl.exe
*_MYTOOLCHAIN_AARCH64_VFRPP_PATH        == $(VS2019_BIN_AARCH64)\cl.exe
*_MYTOOLCHAIN_AARCH64_SLINK_PATH        == $(VS2019_BIN_AARCH64)\lib.exe
*_MYTOOLCHAIN_AARCH64_DLINK_PATH        == $(VS2019_BIN_AARCH64)\link.exe
*_MYTOOLCHAIN_AARCH64_APP_PATH          == $(VS2019_BIN_AARCH64)\cl.exe
*_MYTOOLCHAIN_AARCH64_PP_PATH           == $(VS2019_BIN_AARCH64)\cl.exe
*_MYTOOLCHAIN_AARCH64_ASM_PATH          == $(VS2019_BIN_AARCH64)\armasm64.exe
*_MYTOOLCHAIN_AARCH64_ASLCC_PATH        == $(VS2019_BIN_AARCH64)\cl.exe
*_MYTOOLCHAIN_AARCH64_ASLPP_PATH        == $(VS2019_BIN_AARCH64)\cl.exe
*_MYTOOLCHAIN_AARCH64_ASLDLINK_PATH     == $(VS2019_BIN_AARCH64)\link.exe

  DEBUG_MYTOOLCHAIN_AARCH64_CC_FLAGS    == /nologo /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1b2 /GL /FIAutoGen.h /EHs-c- /GR- /GF /Gy /Zi /Gw /Oi-
RELEASE_MYTOOLCHAIN_AARCH64_CC_FLAGS    == /nologo /c /WX /GS- /W4 /Gs32768 /D UNICODE /O1b2 /GL /FIAutoGen.h /EHs-c- /GR- /GF /Gw /Oi-
NOOPT_MYTOOLCHAIN_AARCH64_CC_FLAGS      == /nologo /c /WX /GS- /W4 /Gs32768 /D UNICODE /FIAutoGen.h /EHs-c- /GR- /GF /Gy /Zi /Od /Oi-

  DEBUG_MYTOOLCHAIN_AARCH64_ASM_FLAGS   == /nologo /g
RELEASE_MYTOOLCHAIN_AARCH64_ASM_FLAGS   == /nologo
NOOPT_MYTOOLCHAIN_AARCH64_ASM_FLAGS     == /nologo

  DEBUG_MYTOOLCHAIN_AARCH64_DLINK_FLAGS == /NOLOGO /NODEFAULTLIB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:ARM64 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /DRIVER /DEBUG
RELEASE_MYTOOLCHAIN_AARCH64_DLINK_FLAGS == /NOLOGO /NODEFAULTLIB /IGNORE:4001 /IGNORE:4254 /OPT:REF /OPT:ICF=10 /MAP /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:ARM64 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /DRIVER /MERGE:.rdata=.data
NOOPT_MYTOOLCHAIN_AARCH64_DLINK_FLAGS   == /NOLOGO /NODEFAULTLIB /IGNORE:4001 /OPT:REF /OPT:ICF=10 /MAP /SECTION:.xdata,D /SECTION:.pdata,D /MACHINE:ARM64 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /SAFESEH:NO /DRIVER /DEBUG

##################
# EBC definitions
##################
*_MYTOOLCHAIN_EBC_*_FAMILY            == INTEL

*_MYTOOLCHAIN_EBC_PP_PATH             == $(EBC_BINX86)\iec.exe
*_MYTOOLCHAIN_EBC_VFRPP_PATH          == $(EBC_BINX86)\iec.exe
*_MYTOOLCHAIN_EBC_CC_PATH             == $(EBC_BINX86)\iec.exe
*_MYTOOLCHAIN_EBC_SLINK_PATH          == $(VS2019_BIN_IA32)\link.exe
*_MYTOOLCHAIN_EBC_DLINK_PATH          == $(VS2019_BIN_IA32)\link.exe

*_MYTOOLCHAIN_EBC_PP_FLAGS            == /nologo /E /TC /FIAutoGen.h
*_MYTOOLCHAIN_EBC_CC_FLAGS            == /nologo /c /WX /W3 /FIAutoGen.h /D$(MODULE_ENTRY_POINT)=$(ARCH_ENTRY_POINT)
*_MYTOOLCHAIN_EBC_VFRPP_FLAGS         == /nologo /E /TC /DVFRCOMPILE /FI$(MODULE_NAME)StrDefs.h
*_MYTOOLCHAIN_EBC_SLINK_FLAGS         == /lib /NOLOGO /MACHINE:EBC
*_MYTOOLCHAIN_EBC_DLINK_FLAGS         == "C:\Program Files (x86)\Intel\EBC\Lib\EbcLib.lib" /NOLOGO /NODEFAULTLIB /MACHINE:EBC /OPT:REF /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER /MAP /ALIGN:32 /DRIVER
