/** @file

  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MTRR_SUPPORT_H_
#define _MTRR_SUPPORT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/MtrrLib.h>
#include <HostTest/UnitTestHostBaseLib.h>

#include <Register/ArchitecturalMsr.h>
#include <Register/Cpuid.h>
#include <Register/Msr.h>

#define UNIT_TEST_APP_NAME        "MtrrLib Unit Tests"
#define UNIT_TEST_APP_VERSION     "1.0"

#define TestIteration  10

#define TEST_VariableMtrrCount  10
#define TEST_NumberOfReservedVariableMtrrs 0
#define NotSpecifiedWithCurrentMtrrSetting 8
#define SCRATCH_BUFFER_SIZE           SIZE_16KB

#define TEST_PhysicalAddressBits 36
extern MTRR_MEMORY_CACHE_TYPE TEST_DefaultCacheType;

extern MTRR_MEMORY_RANGE *mActualRanges;
extern UINT32 mActualRangesCount;
extern UINT32 mActualMtrrsCount;


extern UINT32 MtrrCacheTypeValues[5];

extern CHAR8 *CacheTypeFullNames[9];

VOID
GenerateValidAndConfigurableMtrrPairs (
  IN OUT MTRR_VARIABLE_SETTING **VariableMtrrSettings,
  IN OUT MTRR_MEMORY_RANGE **RawMtrrMemoryRanges,
  IN UINT32 UcCount,
  IN UINT32 WtCount,
  IN UINT32 WbCount,
  IN UINT32 WpCount,
  IN UINT32 WcCount
  );

VOID
GenerateInvalidMemoryLayout (
  IN OUT MTRR_MEMORY_RANGE **EffectiveMemoryRanges,
  IN UINT32 RangesCount
  );

VOID
GetEffectiveMemoryRanges (
  IN MTRR_MEMORY_RANGE *RawMtrrMemoryRanges,
  IN UINT32 RawMtrrMemoryRangeCount,
  OUT MTRR_MEMORY_RANGE **EffectiveMtrrMemoryRanges,
  OUT UINT32 *EffectiveMtrrMemoryRangeCount
  );

VOID
DumpMtrrOrRanges (
  IN MTRR_VARIABLE_SETTING *MtrrPairs,
  IN UINT32 TotalMtrrCount,
  IN MTRR_MEMORY_RANGE *Ranges,
  IN UINT32 TotalRangesCount
  );

VOID
DumpAllRangePiecesEndPoints (
  IN UINT64 *AllRangePiecesEndPoints,
  IN UINT32 AllRangePiecesEndPointCount
  );

VOID
CollectRawMtrrRangesEndpointsAndSortAndRemoveDuplicates (
  IN UINT64 *AllEndPointsInclusive,
  IN UINT32 *pAllEndPointsCount,
  IN MTRR_MEMORY_RANGE *RawMtrrMemoryRanges,
  IN UINT32 RawMtrrMemoryRangesCount
  );

VOID
DumpEffectiveRangesCArrayStyle (
  IN MTRR_MEMORY_RANGE *EffectiveRanges,
  IN UINT32 EffectiveRangesCount
  );

VOID
CollectTestResult (
  VOID
  );

#endif
