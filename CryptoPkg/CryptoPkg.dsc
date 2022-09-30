## @file
#  Cryptographic Library Package for UEFI Security Implementation.
#  PEIM, DXE Driver, and SMM Driver with all crypto services enabled.
#
#  Copyright (c) 2009 - 2021, Intel Corporation. All rights reserved.<BR>
#  Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = CryptoPkg
  PLATFORM_GUID                  = E1063286-6C8C-4c25-AEF0-67A9A5B6E6B6
  PLATFORM_VERSION               = 0.98
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/CryptoPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64|ARM|AARCH64|RISCV64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

  #
  # Flavor of PEI, DXE, SMM modules to build.
  # Must be one of ALL, NONE, MIN_PEI, MIN_DXE_MIN_SMM, TARGET_UINT_TESTS.
  # Default is ALL that is used for package build verification.
  #   ALL               - Build PEIM, DXE, and SMM drivers.  Protocols and PPIs
  #                       publish all services.
  #   NONE              - Build PEIM, DXE, and SMM drivers.  Protocols and PPIs
  #                       publish no services.  Used to verify compiler/linker
  #                       optimizations are working correctly.
  #   MIN_PEI           - Build PEIM with PPI that publishes minimum required
  #                       services.
  #   MIN_DXE_MIN_SMM   - Build DXE and SMM drivers with Protocols that publish
  #                       minimum required services.
  #   TARGET_UNIT_TESTS - Build target-based unit tests
  #
  DEFINE CRYPTO_SERVICES = ALL
!if $(CRYPTO_SERVICES) IN "ALL NONE MIN_PEI MIN_DXE_MIN_SMM TARGET_UNIT_TESTS"
!else
  !error CRYPTO_SERVICES must be set to one of ALL NONE MIN_PEI MIN_DXE_MIN_SMM TARGET_UNIT_TESTS.
!endif

!if $(CRYPTO_SERVICES) == TARGET_UNIT_TESTS
  !include UnitTestFrameworkPkg/UnitTestFrameworkPkgTarget.dsc.inc
!endif

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses]
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibNull/BaseCryptLibNull.inf
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
  HashApiLib|CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  RngLib|MdePkg/Library/BaseRngLibNull/BaseRngLibNull.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf

[LibraryClasses.ARM, LibraryClasses.AARCH64]
  ArmLib|ArmPkg/Library/ArmLib/ArmBaseLib.inf
  #
  # It is not possible to prevent the ARM compiler for generic intrinsic functions.
  # This library provides the instrinsic functions generate by a given compiler.
  # [LibraryClasses.ARM, LibraryClasses.AARCH64] and NULL mean link this library
  # into all ARM and AARCH64 images.
  #
  NULL|ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf

  # Add support for stack protector
  NULL|MdePkg/Library/BaseStackCheckLib/BaseStackCheckLib.inf

[LibraryClasses.common.PEIM]
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf
  MemoryAllocationLib|MdePkg/Library/SmmMemoryAllocationLib/SmmMemoryAllocationLib.inf
  MmServicesTableLib|MdePkg/Library/MmServicesTableLib/MmServicesTableLib.inf

[LibraryClasses]
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  DebugLib|MdeModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf

[LibraryClasses.ARM]
  ArmSoftFloatLib|ArmPkg/Library/ArmSoftFloatLib/ArmSoftFloatLib.inf

[LibraryClasses.common.SEC]
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SecCryptLib.inf

[LibraryClasses.common.PEIM]
  PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf

[LibraryClasses.IA32.PEIM, LibraryClasses.X64.PEIM]
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLibIdt/PeiServicesTablePointerLibIdt.inf

[LibraryClasses.ARM.PEIM, LibraryClasses.AARCH64.PEIM]
  PeiServicesTablePointerLib|ArmPkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf

[LibraryClasses.common.DXE_DRIVER]
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  ReportStatusCodeLib|MdeModulePkg/Library/SmmReportStatusCodeLib/SmmReportStatusCodeLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################
[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x0f
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000000
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x06

!if $(CRYPTO_SERVICES) == NONE
[Defines]
  DEFINE  PEI_CRYPTO_GUID       = AFD9168C-E233-4EDB-B8CE-62937241D819
  DEFINE  PEI_STD_GUID          = C10007C4-FF93-4DAA-987A-71D399488E9C
  DEFINE  PEI_FULL_GUID         = 46A8A0AF-ED6D-400B-B0AC-4E6F9F0F2979
  DEFINE  PEI_STD_ACCEL_GUID    = D0C0AF35-4B79-4144-AE52-47792E21F8E2
  DEFINE  PEI_FULL_ACCEL_GUID   = 0C5EC6EA-B5F0-4156-A74E-F2FAFBA63338
  DEFINE  DXE_CRYPTO_GUID       = 4BA89D0C-0745-403F-B32B-D3F305872CF6
  DEFINE  DXE_STD_GUID          = 35E77B2E-C29F-4C53-BEB1-9B9E249B2ADF
  DEFINE  DXE_FULL_GUID         = D03FFB4F-DCDD-4376-9E42-C75851BDC361
  DEFINE  DXE_STD_ACCEL_GUID    = FF1C10D7-058A-456E-9AC7-ACA444C57528
  DEFINE  DXE_FULL_ACCEL_GUID   = 0B4A47FF-6DA0-435E-9DC8-27E4263AED60
  DEFINE  SMM_CRYPTO_GUID       = 05244BD7-E667-426E-AA16-7ACF40E86C36
  DEFINE  SMM_STD_GUID          = FE3061CE-CFA1-4DC8-ACB4-BDC94F65435F
  DEFINE  SMM_FULL_GUID         = 272E7DBA-4D56-4364-9F29-4AD198FF99C8
  DEFINE  SMM_STD_ACCEL_GUID    = A977B43A-AB2B-4ACB-A9A5-F5086139EA63
  DEFINE  SMM_FULL_ACCEL_GUID   = 0272F958-66C5-4053-9F35-65E2B8F91E0C
!endif
!if $(CRYPTO_SERVICES) IN "ALL TARGET_UINT_TESTS"
[Defines]
  DEFINE  PEI_CRYPTO_GUID      = C693A250-6B36-49B9-B7F3-7283F8136A72
  DEFINE  PEI_STD_GUID         = EBD49F5C-6D8B-40D1-A56D-9AFA485A8661
  DEFINE  PEI_FULL_GUID        = D51FCE59-6860-49C0-9B35-984470735D17
  DEFINE  PEI_STD_ACCEL_GUID   = DCC9CB49-7BE2-47C6-864E-6DCC932360F9
  DEFINE  PEI_FULL_ACCEL_GUID  = A10827AD-7598-4955-B661-52EE2B62B057
  DEFINE  DXE_CRYPTO_GUID      = 31C17C54-325D-47D5-8622-888098F10E44
  DEFINE  DXE_STD_GUID         = ADD6D05A-52A2-437B-98E7-DBFDA89352CD
  DEFINE  DXE_FULL_GUID        = AA83B296-F6EA-447F-B013-E80E98629CF8
  DEFINE  DXE_STD_ACCEL_GUID   = 9FBDAD27-910C-4229-9EFF-A93BB5FE18C6
  DEFINE  DXE_FULL_ACCEL_GUID  = 41A491D1-A972-468B-A299-DABF415A43B7
  DEFINE  SMM_CRYPTO_GUID      = 1A1C9E13-5722-4636-AB73-31328EDE8BAF
  DEFINE  SMM_STD_GUID         = E4D7D1E3-E886-4412-A442-EFD6F2502DD3
  DEFINE  SMM_FULL_GUID        = 1930CE7E-6598-48ED-8AB1-EBE7E85EC254
  DEFINE  SMM_STD_ACCEL_GUID   = 828959D3-CEA6-4B79-B1FC-5AFA0D7F2144
  DEFINE  SMM_FULL_ACCEL_GUID  = C1760694-AB3A-4532-8C6D-52D8F86EB1AA

[PcdsFixedAtBuild]
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.HmacSha256.Family                        | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.HmacSha384.Family                        | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Family                              | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Dh.Family                                | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Random.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Family                               | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha1.Family                              | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha256.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha384.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha512.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.X509.Family                              | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.GetContextSize              | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.Init                        | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.CbcEncrypt                  | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.CbcDecrypt                  | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Arc4.Family                              | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sm3.Family                               | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Hkdf.Family                              | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Tls.Family                               | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.TlsSet.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.TlsGet.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.RsaPss.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.ParallelHash.Family                      | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.AeadAesGcm.Family                        | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Bn.Family                                | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Ec.Family                                | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
!endif
!if $(CRYPTO_SERVICES) == MIN_PEI
[Defines]
  DEFINE  PEI_CRYPTO_GUID       = D8B06EBF-771F-4A8A-BED6-0E454F1783BB
  DEFINE  PEI_STD_GUID          = D58D4AE2-29F2-46AF-9344-8DD5D6153598
  DEFINE  PEI_FULL_GUID         = F965717E-46E9-412B-A391-C4B4B0C430BC
  DEFINE  PEI_STD_ACCEL_GUID    = D9DD7EFD-DD61-4334-9736-28FC854E5425
  DEFINE  PEI_FULL_ACCEL_GUID   = 28CECE3C-0391-4ECD-B18B-74A45B6D63DF
  DEFINE  DXE_CRYPTO_GUID       = EDAD3C66-59E4-4D4E-84C5-B1C3EA7E616C
  DEFINE  DXE_STD_GUID          = 35870E9F-DF8F-48D0-ADE7-357350201CF2
  DEFINE  DXE_FULL_GUID         = 16C77F03-9209-4969-9D84-6A335619BCE4
  DEFINE  DXE_STD_ACCEL_GUID    = BFC8E7F3-186D-4EB5-8670-A36D6109968B
  DEFINE  DXE_FULL_ACCEL_GUID   = BDF7DDCA-F865-4B56-A1C4-4004F2D4913E
  DEFINE  SMM_CRYPTO_GUID       = 171B20D5-8135-4327-8510-EAC9C8FE0022
  DEFINE  SMM_STD_GUID          = 1809C064-8D04-4855-8E2F-EEE10B80EC8D
  DEFINE  SMM_FULL_GUID         = 5B3CF9A4-9745-4612-B07A-E34A407225D7
  DEFINE  SMM_STD_ACCEL_GUID    = 609E3F31-5E69-4725-8798-0062ABF9C68A
  DEFINE  SMM_FULL_ACCEL_GUID   = 69D4ADEE-CE5E-4623-A11D-124718F71179

[PcdsFixedAtBuild]
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.HmacSha256.Family               | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.HmacSha384.Family               | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha1.Family                     | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha256.Family                   | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha384.Family                   | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha512.Family                   | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sm3.Family                      | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.Pkcs1Verify        | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.New                | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.Free               | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.SetKey             | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs5HashPassword | TRUE
!endif
!if $(CRYPTO_SERVICES) == MIN_DXE_MIN_SMM
[Defines]
  DEFINE  PEI_CRYPTO_GUID       = A0C9D1A7-070E-4D67-B932-103B9E5A958C
  DEFINE  PEI_STD_GUID          = 095AF351-D872-498A-8B56-EAE1AF4E432E
  DEFINE  PEI_FULL_GUID         = 87AA1CB3-AFC2-412B-A44C-E802EF287F08
  DEFINE  PEI_STD_ACCEL_GUID    = 8943EF16-0CB8-4DF4-85FF-96B59539936B
  DEFINE  PEI_FULL_ACCEL_GUID   = 04BF1E61-A625-4537-9675-10FB4AA3DA4A
  DEFINE  DXE_CRYPTO_GUID       = D34A2B8C-1F18-4C31-9632-89244FF194D8
  DEFINE  DXE_STD_GUID          = FEDC475F-86A8-48F2-9FE9-A73704B3F26C
  DEFINE  DXE_FULL_GUID         = E4482D13-5918-442A-A65B-F22271B76CE3
  DEFINE  DXE_STD_ACCEL_GUID    = 4A2456FC-8F2B-4165-B2C3-B0625A75CC7E
  DEFINE  DXE_FULL_ACCEL_GUID   = FD169D32-67FE-43EE-B7E1-43D3AEE3CF00
  DEFINE  SMM_CRYPTO_GUID       = 2A2131A1-BA39-4E06-8B10-48F9FF4B7E2C
  DEFINE  SMM_STD_GUID          = CF80F52B-F561-4AA0-826F-112D89BE0621
  DEFINE  SMM_FULL_GUID         = F8243334-543A-4ED3-89C5-11B17FDA1AE0
  DEFINE  SMM_STD_ACCEL_GUID    = 913E6BAF-15D9-4EEF-BA75-87918C1FFD24
  DEFINE  SMM_FULL_ACCEL_GUID   = 74F4D471-CEA6-4738-8416-5EED2B36D67C

[PcdsFixedAtBuild]
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.HmacSha256.Family                        | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.HmacSha384.Family                        | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs1v2Encrypt             | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs5HashPassword          | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs7Verify                | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.VerifyEKUsInPkcs7Signature | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs7GetSigners            | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs7FreeSigners           | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.AuthenticodeVerify         | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Random.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.Pkcs1Verify                 | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.New                         | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.Free                        | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.SetKey                      | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.GetPublicKeyFromX509        | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha1.Family                              | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha256.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha256.Services.HashAll                  | FALSE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.X509.Services.GetSubjectName             | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.X509.Services.GetCommonName              | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.X509.Services.GetOrganizationName        | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.X509.Services.GetTBSCert                 | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Tls.Family                               | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.TlsSet.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.TlsGet.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.GetContextSize              | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.Init                        | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.CbcEncrypt                  | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.CbcDecrypt                  | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.AeadAesGcm.Services.Encrypt              | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.AeadAesGcm.Services.Decrypt              | TRUE
!endif

###################################################################################################
#
# Components Section - list of the modules and components that will be processed by compilation
#                      tools and the EDK II tools to generate PE32/PE32+/Coff image files.
#
# Note: The EDK II DSC file is not used to specify how compiled binary images get placed
#       into firmware volume images. This section is just a list of modules to compile from
#       source into UEFI-compliant binaries.
#       It is the FDF file that contains information on combining binary files into firmware
#       volume images, whose concept is beyond UEFI and is described in PI specification.
#       Binary modules do not need to be listed in this section, as they should be
#       specified in the FDF file. For example: Shell binary (Shell_Full.efi), FAT binary (Fat.efi),
#       Logo (Logo.bmp), and etc.
#       There may also be modules listed in this section that are not required in the FDF file,
#       When a module listed here is excluded from FDF file, then UEFI-compliant binary will be
#       generated for it, but the binary will not be put into any firmware volume.
#
###################################################################################################
!if $(CRYPTO_SERVICES) == TARGET_UNIT_TESTS
[Components.IA32, Components.X64, Components.ARM, Components.AARCH64]
  #
  # Target based unit tests
  #
  CryptoPkg/Test/UnitTest/Library/BaseCryptLib/TestBaseCryptLibShell.inf {
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFull.inf
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
      TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
      UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
      RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
    <BuildOptions>
      MSFT:*_*_*_DLINK_FLAGS     = /ALIGN:4096 /FILEALIGN:4096 /SUBSYSTEM:CONSOLE
      MSFT:DEBUG_*_*_DLINK_FLAGS = /EXPORT:InitializeDriver=$(IMAGE_ENTRY_POINT) /BASE:0x10000
      MSFT:DEBUG_*_*_DLINK_FLAGS = /EXPORT:InitializeDriver=$(IMAGE_ENTRY_POINT) /BASE:0x10000
      MSFT:NOOPT_*_*_DLINK_FLAGS = /EXPORT:InitializeDriver=$(IMAGE_ENTRY_POINT) /BASE:0x10000
  }
!else
[Components]
  #
  # Common libraries
  #
  CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  CryptoPkg/Library/BaseCryptLib/SecCryptLib.inf
  CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf
  CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
  CryptoPkg/Library/BaseCryptLib/RuntimeCryptLib.inf
  CryptoPkg/Library/BaseCryptLibNull/BaseCryptLibNull.inf
  CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  CryptoPkg/Library/TlsLib/TlsLib.inf
  CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
  CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf
  CryptoPkg/Library/OpensslLib/OpensslLib.inf
  CryptoPkg/Library/OpensslLib/OpensslLibFull.inf
  CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  CryptoPkg/Library/BaseCryptLibOnProtocolPpi/PeiCryptLib.inf
  CryptoPkg/Library/BaseCryptLibOnProtocolPpi/DxeCryptLib.inf
  CryptoPkg/Library/BaseCryptLibOnProtocolPpi/SmmCryptLib.inf

[Components.IA32, Components.X64]
  #
  # IA32/X64 specific libraries
  #
  CryptoPkg/Library/OpensslLib/OpensslLibAccel.inf
  CryptoPkg/Library/OpensslLib/OpensslLibFullAccel.inf

[Components.IA32, Components.X64, Components.ARM, Components.AARCH64]
  #
  # CryptoPei with OpensslLib instance without SSL or EC services
  #
  CryptoPkg/Driver/CryptoPei.inf {
    <Defines>
      FILE_GUID = $(PEI_CRYPTO_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf
  }
  #
  # CryptoPei with OpensslLib instance without EC services
  #
  CryptoPkg/Driver/CryptoPei.inf {
    <Defines>
      FILE_GUID = $(PEI_STD_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  }
  #
  # CryptoPei with OpensslLib instance with all services
  #
  CryptoPkg/Driver/CryptoPei.inf {
    <Defines>
      FILE_GUID = $(PEI_FULL_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFull.inf
  }

[Components.IA32, Components.X64]
  #
  # CryptoPei with IA32/X64 performance optimized OpensslLib instance without EC services
  # IA32/X64 assembly optimizations required larger alignments
  #
  CryptoPkg/Driver/CryptoPei.inf {
    <Defines>
      FILE_GUID = $(PEI_STD_ACCEL_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibAccel.inf
    <BuildOptions>
      MSFT:*_*_IA32_DLINK_FLAGS = /ALIGN:64
      MSFT:*_*_X64_DLINK_FLAGS  = /ALIGN:256
  }

  #
  # CryptoPei with IA32/X64 performance optimized OpensslLib instance all services
  # IA32/X64 assembly optimizations required larger alignments
  #
  CryptoPkg/Driver/CryptoPei.inf {
    <Defines>
      FILE_GUID = $(PEI_FULL_ACCEL_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFullAccel.inf
    <BuildOptions>
      MSFT:*_*_IA32_DLINK_FLAGS = /ALIGN:64
      MSFT:*_*_X64_DLINK_FLAGS  = /ALIGN:256
  }

[Components.IA32, Components.X64, Components.AARCH64]
  #
  # CryptoDxe with OpensslLib instance with no SSL or EC services
  #
  CryptoPkg/Driver/CryptoDxe.inf {
    <Defines>
      FILE_GUID = $(DXE_CRYPTO_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf
  }
  #
  # CryptoDxe with OpensslLib instance with no EC services
  #
  CryptoPkg/Driver/CryptoDxe.inf {
    <Defines>
      FILE_GUID = $(DXE_STD_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  }
  #
  # CryptoDxe with OpensslLib instance with all services
  #
  CryptoPkg/Driver/CryptoDxe.inf {
    <Defines>
      FILE_GUID = $(DXE_FULL_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFull.inf
  }

[Components.IA32, Components.X64]
  #
  # CryptoDxe with IA32/X64 performance optimized OpensslLib instance with no EC services
  # with TLS feature enabled.
  # IA32/X64 assembly optimizations required larger alignments
  #
  CryptoPkg/Driver/CryptoDxe.inf {
    <Defines>
      FILE_GUID = $(DXE_STD_ACCEL_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibAccel.inf
    <BuildOptions>
      MSFT:*_*_IA32_DLINK_FLAGS = /ALIGN:64
      MSFT:*_*_X64_DLINK_FLAGS  = /ALIGN:256
  }
  #
  # CryptoDxe with IA32/X64 performance optimized OpensslLib instance with all services.
  # IA32/X64 assembly optimizations required larger alignments
  #
  CryptoPkg/Driver/CryptoDxe.inf {
    <Defines>
      FILE_GUID = $(DXE_FULL_ACCEL_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFullAccel.inf
    <BuildOptions>
      MSFT:*_*_IA32_DLINK_FLAGS = /ALIGN:64
      MSFT:*_*_X64_DLINK_FLAGS  = /ALIGN:256
  }
  #
  # CryptoSmm with OpensslLib instance with no SSL or EC services
  #
  CryptoPkg/Driver/CryptoSmm.inf {
    <Defines>
      FILE_GUID = $(SMM_CRYPTO_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf
  }
  #
  # CryptoSmm with OpensslLib instance with no SSL services
  #
  CryptoPkg/Driver/CryptoSmm.inf {
    <Defines>
      FILE_GUID = $(SMM_STD_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  }
  #
  # CryptoSmm with OpensslLib instance with no all services
  #
  CryptoPkg/Driver/CryptoSmm.inf {
    <Defines>
      FILE_GUID = $(SMM_FULL_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFull.inf
  }
  #
  # CryptoSmm with IA32/X64 performance optimized OpensslLib instance with no EC services
  # IA32/X64 assembly optimizations required larger alignments
  #
  CryptoPkg/Driver/CryptoSmm.inf {
    <Defines>
      FILE_GUID = $(SMM_STD_ACCEL_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibAccel.inf
    <BuildOptions>
      MSFT:*_*_IA32_DLINK_FLAGS = /ALIGN:64
      MSFT:*_*_X64_DLINK_FLAGS  = /ALIGN:256
  }
  #
  # CryptoSmm with IA32/X64 performance optimized OpensslLib instance with all services
  # IA32/X64 assembly optimizations required larger alignments
  #
  CryptoPkg/Driver/CryptoSmm.inf {
    <Defines>
      FILE_GUID = $(SMM_FULL_ACCEL_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFullAccel.inf
    <BuildOptions>
      MSFT:*_*_IA32_DLINK_FLAGS = /ALIGN:64
      MSFT:*_*_X64_DLINK_FLAGS  = /ALIGN:256
  }
!endif

[BuildOptions]
  RELEASE_*_*_CC_FLAGS = -DMDEPKG_NDEBUG
  *_*_*_CC_FLAGS       = -D DISABLE_NEW_DEPRECATED_INTERFACES
