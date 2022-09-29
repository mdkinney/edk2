## @file
# CryptoPkg DSC file used to build host-based unit tests.
#
# Copyright (c) Microsoft Corporation.<BR>
# Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME           = CryptoPkgHostTest
  PLATFORM_GUID           = C7F97D6D-54AC-45A9-8197-CC99B20CC7EC
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/CryptoPkg/HostTest
  SUPPORTED_ARCHITECTURES = IA32|X64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[LibraryClasses]
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFull.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/UnitTestHostBaseCryptLib.inf
  MmServicesTableLib|MdePkg/Library/MmServicesTableLib/MmServicesTableLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf

[LibraryClasses.AARCH64, LibraryClasses.ARM]
  RngLib|MdePkg/Library/BaseRngLibNull/BaseRngLibNull.inf

[LibraryClasses.X64, LibraryClasses.IA32]
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf

[PcdsFixedAtBuild]
  # Enable all crypto services for unit tests
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.HmacSha256.Family                        | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.HmacSha384.Family                        | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Md5.Family                               | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
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

[Components]
  #
  # Build HOST_APPLICATION that tests the SampleUnitTest
  #
  CryptoPkg/Test/UnitTest/Library/BaseCryptLib/TestBaseCryptLibHost.inf
  CryptoPkg/Test/UnitTest/Library/BaseCryptLib/TestBaseCryptLibHostAccel.inf {
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFullAccel.inf
  }

[BuildOptions]
  *_*_*_CC_FLAGS       = -D DISABLE_NEW_DEPRECATED_INTERFACES
