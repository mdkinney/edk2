/** @file
  Internal include file for TlsLib.

Copyright (c) 2016 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __INTERNAL_TLS_LIB_H__
#define __INTERNAL_TLS_LIB_H__

#undef _WIN32
#undef _WIN64

#include <Library/BaseCryptLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SafeIntLib.h>
#include <Library/PcdLib.h>
#include <Pcd/PcdCryptoServiceFamilyEnable.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

/**
  A macro used to retrieve the FixedAtBuild PcdCryptoServiceFamilyEnable with a
  typecast to its associated structure type PCD_CRYPTO_SERVICE_FAMILY_ENABLE.
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

typedef struct {
  //
  // Main SSL Connection which is created by a server or a client
  // per established connection.
  //
  SSL    *Ssl;
  //
  // Memory BIO for the TLS/SSL Reading operations.
  //
  BIO    *InBio;
  //
  // Memory BIO for the TLS/SSL Writing operations.
  //
  BIO    *OutBio;
} TLS_CONNECTION;

#endif
