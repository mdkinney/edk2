/** @file
  Internal include file for BaseCryptLib.

Copyright (c) 2010 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __INTERNAL_CRYPT_LIB_H__
#define __INTERNAL_CRYPT_LIB_H__

#undef _WIN32
#undef _WIN64

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/PcdLib.h>
#include <Pcd/PcdCryptoServiceFamilyEnable.h>

#include "CrtLibSupport.h"

#include <openssl/opensslv.h>

#if OPENSSL_VERSION_NUMBER < 0x10100000L
#define OBJ_get0_data(o)  ((o)->data)
#define OBJ_length(o)     ((o)->length)
#endif


/**
  A macro used to retrieve the FixedAtBuild PcdCryptoServiceFamilyEnable with a
  typecast to its associcted structure type PCD_CRYPTO_SERVICE_FAMILY_ENABLE.
**/
#define EDKII_CRYPTO_PCD  ((const PCD_CRYPTO_SERVICE_FAMILY_ENABLE *) \
  (FixedPcdGetPtr (PcdCryptoServiceFamilyEnable)))

/**
  A macro used to check if an EDK II Crypto Service is enabled.  If the
  service is not enabled, then ASSERT() with the name service that is not
  enabled from PCD_CRYPTO_SERVICE_FAMILY_ENABLE and return the value
  specified if the service is disabled.  If the service is disabled and
  an optimizing compiler/linker is used, the the statements that follow
  this macro would be removed.
**/
#define IS_EDKII_CRYPTO_SERVICE_ENABLED(Service, DisabledReturnValue) \
  if (!EDKII_CRYPTO_PCD->Service) {                                   \
    ASSERT (EDKII_CRYPTO_PCD->Service);                               \
    return DisabledReturnValue;                                       \
  }

/**
  Check input P7Data is a wrapped ContentInfo structure or not. If not construct
  a new structure to wrap P7Data.

  Caution: This function may receive untrusted input.
  UEFI Authenticated Variable is external input, so this function will do basic
  check for PKCS#7 data structure.

  @param[in]  P7Data       Pointer to the PKCS#7 message to verify.
  @param[in]  P7Length     Length of the PKCS#7 message in bytes.
  @param[out] WrapFlag     If TRUE P7Data is a ContentInfo structure, otherwise
                           return FALSE.
  @param[out] WrapData     If return status of this function is TRUE:
                           1) when WrapFlag is TRUE, pointer to P7Data.
                           2) when WrapFlag is FALSE, pointer to a new ContentInfo
                           structure. It's caller's responsibility to free this
                           buffer.
  @param[out] WrapDataSize Length of ContentInfo structure in bytes.

  @retval     TRUE         The operation is finished successfully.
  @retval     FALSE        The operation is failed due to lack of resources.

**/
BOOLEAN
WrapPkcs7Data (
  IN  CONST UINT8  *P7Data,
  IN  UINTN        P7Length,
  OUT BOOLEAN      *WrapFlag,
  OUT UINT8        **WrapData,
  OUT UINTN        *WrapDataSize
  );

#endif
