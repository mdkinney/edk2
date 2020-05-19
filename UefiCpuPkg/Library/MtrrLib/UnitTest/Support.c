/** @file
  Unit tests of the MtrrLib instance of the MtrrLib class

  Copyright (c) 2018 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MtrrLibUnitTest.h"

MTRR_MEMORY_CACHE_TYPE TEST_DefaultCacheType = CacheUncacheable;

CHAR8 *CacheTypeShortNames[9] = { "UC", "WC", "R*", "R*", "WT", "WP", "WB", "I*", "NotSpecifiedWithCurrentMtrrSetting" };// R*=Reserved, I*=Invalid
CHAR8 *CacheTypeFullNames[9] = { "CacheUncacheable", "CacheWriteCombining", "Reserved", "Reserved", "CacheWriteThrough", "CacheWriteProtected", "CacheWriteBack", "CacheInvalid", "NotSpecifiedWithCurrentMtrrSetting" };

UINT32 MtrrCacheTypeValues[5] = { 0, 1, 4, 5, 6 };


MTRR_MEMORY_RANGE *mActualRanges;
UINT32 mActualRangesCount;
UINT32 mActualMtrrsCount;

VOID
MtrrLibInitializeMtrrMask (
  OUT UINT64 *MtrrValidBitsMask,
  OUT UINT64 *MtrrValidAddressMask
  );
UINT32
MtrrLibGetRawVariableRanges (
  IN  MTRR_VARIABLE_SETTINGS  *VariableSettings,
  IN  UINTN                   VariableMtrrCount,
  IN  UINT64                  MtrrValidBitsMask,
  IN  UINT64                  MtrrValidAddressMask,
  OUT MTRR_MEMORY_RANGE       *VariableMtrr
  );
RETURN_STATUS
MtrrLibApplyVariableMtrrs (
  IN     CONST MTRR_MEMORY_RANGE *VariableMtrr,
  IN     UINT32                  VariableMtrrCount,
  IN OUT MTRR_MEMORY_RANGE       *Ranges,
  IN     UINTN                   RangeCapacity,
  IN OUT UINTN                   *RangeCount
  );
RETURN_STATUS
MtrrLibApplyFixedMtrrs (
  IN     MTRR_FIXED_SETTINGS  *Fixed,
  IN OUT MTRR_MEMORY_RANGE    *Ranges,
  IN     UINTN                RangeCapacity,
  IN OUT UINTN                *RangeCount
  );

VOID
CollectTestResult (
  VOID
  )
{
    MTRR_SETTINGS     LocalMtrrs;
    MTRR_SETTINGS     *Mtrrs;
    UINTN             Index;
    UINTN             RangeCount;
    UINT64            MtrrValidBitsMask;
    UINT64            MtrrValidAddressMask;
    UINT32            VariableMtrrCount;
    MTRR_MEMORY_RANGE Ranges[
      11 * sizeof (UINT64) + 2 * ARRAY_SIZE (Mtrrs->Variables.Mtrr) + 1
      ];
    MTRR_MEMORY_RANGE RawVariableRanges[ARRAY_SIZE (Mtrrs->Variables.Mtrr)];

    VariableMtrrCount = GetVariableMtrrCount ();
    MtrrGetAllMtrrs (&LocalMtrrs);
    Mtrrs = &LocalMtrrs;

    // Collect test result.
    mActualMtrrsCount = 0;
    for (Index = 0; Index < VariableMtrrCount; Index++) {
        if (((MSR_IA32_MTRR_PHYSMASK_REGISTER *)&Mtrrs->Variables.Mtrr[Index].Mask)->Bits.V == 0) {
            //
            // If mask is not valid, then do not display range
            //
            continue;
        }
        mActualMtrrsCount++;
    }

    MtrrLibInitializeMtrrMask (&MtrrValidBitsMask, &MtrrValidAddressMask);
    Ranges[0].BaseAddress = 0;
    Ranges[0].Length      = MtrrValidBitsMask + 1;
    Ranges[0].Type        = MtrrGetDefaultMemoryType ();
    RangeCount = 1;

    MtrrLibGetRawVariableRanges (
      &Mtrrs->Variables, VariableMtrrCount,
      MtrrValidBitsMask, MtrrValidAddressMask, RawVariableRanges
      );
    MtrrLibApplyVariableMtrrs (
      RawVariableRanges, VariableMtrrCount,
      Ranges, ARRAY_SIZE (Ranges), &RangeCount
      );

    MtrrLibApplyFixedMtrrs (&Mtrrs->Fixed, Ranges, ARRAY_SIZE (Ranges), &RangeCount);

    mActualRangesCount = (UINT32)RangeCount;
    mActualRanges = calloc(RangeCount, sizeof(MTRR_MEMORY_RANGE));  // Caller to free mActualRanges!
    CopyMem(mActualRanges, Ranges, RangeCount * sizeof(MTRR_MEMORY_RANGE));
}

UINT64
GetBitsMaskForPhysBasePhyMaskNonReservedBits (
  VOID
  )
{
    UINT64 Mask1 = (UINT64)~0;
    UINT64 Mask2 = Mask1 >> (64 - TEST_PhysicalAddressBits);
    UINT64 Mask3 = Mask1 << 12;
    return Mask1 & Mask2 & Mask3;
}


VOID ComposeMtrrPairOfSpecificType
(
  OUT MTRR_VARIABLE_SETTING *MtrrPair,
  IN MTRR_MEMORY_CACHE_TYPE CacheType,
  OUT MTRR_MEMORY_RANGE *MtrrMemoryRange
  )
{
    MSR_IA32_MTRR_PHYSBASE_REGISTER PhysBase;
    MSR_IA32_MTRR_PHYSMASK_REGISTER PhysMask;

    UINT32 AddressBitsWidthBeyond4K = TEST_PhysicalAddressBits - 12;
    UINT64 MaxAddressSpaceLength = ((UINT64)1 << TEST_PhysicalAddressBits);
    UINT64 RangeSize = 0;
    UINT64 RangeBase = 0;
    while (RangeBase < SIZE_1MB || RangeBase > MaxAddressSpaceLength - 1)
    {
        UINT64 RandomRangeSizeIn4KPageUnitAsPowerOf2 = ((UINT64)rand() % (AddressBitsWidthBeyond4K + 1));
        UINT64 RandomRangeSizeAsPowerOf2 = RandomRangeSizeIn4KPageUnitAsPowerOf2 + 12;
        RangeSize = ((UINT64)1) << RandomRangeSizeAsPowerOf2;


        UINT64 n = RandomRangeSizeAsPowerOf2;
        UINT64 max_m = TEST_PhysicalAddressBits;
        UINT64 m = (((UINT64)rand()) % (max_m - n + 1)) + n;

        UINT64 BoundarySegmentCount = (((UINT64)1) << (TEST_PhysicalAddressBits - m));

        UINT64 randomBoundary = (UINT64)rand() % BoundarySegmentCount;
        RangeBase = randomBoundary << m;
    }

    UINT64 PhysBasePhyMaskValidBitsMask = GetBitsMaskForPhysBasePhyMaskNonReservedBits();
    UINT64 BaseField = RangeBase & PhysBasePhyMaskValidBitsMask;
    UINT64 MaskField = ((~RangeSize) + 1) & PhysBasePhyMaskValidBitsMask;

    PhysBase.Uint64 = 0;
    PhysBase.Bits.Type = CacheType;
    PhysBase.Uint64 |= BaseField;
    PhysMask.Uint64 = 0;
    PhysMask.Bits.V = 1;
    PhysMask.Uint64 |= MaskField;

    MtrrPair->Base = PhysBase.Uint64;
    MtrrPair->Mask = PhysMask.Uint64;
    MtrrMemoryRange->BaseAddress = RangeBase;
    MtrrMemoryRange->Length = RangeSize;
    MtrrMemoryRange->Type = CacheType;
}


BOOLEAN
RangeOverlap (
  IN MTRR_MEMORY_RANGE *range1,
  IN MTRR_MEMORY_RANGE *range2
  )
{
    UINT64 base1 = range1->BaseAddress;
    UINT64 top1 = base1 + range1->Length - 1;
    UINT64 base2 = range2->BaseAddress;
    UINT64 top2 = base2 + range2->Length - 1;

    return (base1 <= base2 && top1 >= base2)
        || (base1 >= base2 && top1 <= top2)
        || (base1 <= base2 && top1 >= top2)
        || (base1 <= top2  && top1 >= top2);
}

BOOLEAN
WpOverlapWithWtWb (
  IN MTRR_MEMORY_RANGE *MtrrMemoryRanges,
  IN UINT32 UcCount,
  IN UINT32 WtCount,
  IN UINT32 WbCount,
  IN MTRR_MEMORY_RANGE WpRange
  )
{
    UINT32 i;
    // Check overlap with Wt
    for (i = UcCount; i < UcCount + WtCount; i++)
    {
        if (RangeOverlap(&WpRange, &(MtrrMemoryRanges[i])))
            return TRUE;
    }

    // Check overlap with Wb
    for (i = UcCount + WtCount; i < UcCount + WtCount + WbCount; i++)
    {
        if (RangeOverlap(&WpRange, &(MtrrMemoryRanges[i])))
            return TRUE;
    }
    return FALSE;
}

BOOLEAN
WcOverlapWithWtWbWp (
  IN MTRR_MEMORY_RANGE *MtrrMemoryRanges,
  IN UINT32 UcCount,
  IN UINT32 WtCount,
  IN UINT32 WbCount,
  IN UINT32 WpCount,
  IN MTRR_MEMORY_RANGE WcRange
  )
{
    UINT32 i;
    // Check overlap with Wt
    for (i = UcCount; i < UcCount + WtCount; i++)
    {
        if (RangeOverlap(&WcRange, &(MtrrMemoryRanges[i])))
            return TRUE;
    }

    // Check overlap with Wb
    for (i = UcCount + WtCount; i < UcCount + WtCount + WbCount; i++)
    {
        if (RangeOverlap(&WcRange, &(MtrrMemoryRanges[i])))
            return TRUE;
    }

    // Check overlap with Wp
    for (i = UcCount + WtCount + WbCount; i < UcCount + WtCount + WbCount + WpCount; i++)
    {
        if (RangeOverlap(&WcRange, &(MtrrMemoryRanges[i])))
            return TRUE;
    }
    return FALSE;
}

/*
Raw range is 1:1 with Mtrr
Effecitve range is NOT 1:1 with Mtrr
This function can print both raw and effective ranges.
*/
VOID
DumpMtrrOrRanges (
  IN MTRR_VARIABLE_SETTING *MtrrPairs,
  IN UINT32 TotalMtrrCount,
  IN MTRR_MEMORY_RANGE *Ranges,
  IN UINT32 TotalRangesCount
  )
{
    UINT32 i;
    if (MtrrPairs != NULL)
    {
        for (i = 0; i < TotalMtrrCount; i++)
        {
            DEBUG((DEBUG_ERROR, "\tMtrrPair->Base = 0x%016llx\n", MtrrPairs[i].Base));
            DEBUG((DEBUG_ERROR, "\tMtrrPair->Mask = 0x%016llx\n", MtrrPairs[i].Mask));
        }
    }

    if (Ranges != NULL)
    {
        for (i = 0; i < TotalRangesCount; i++)
        {
            DEBUG((DEBUG_ERROR, "\t[%02d] RangeBase = 0x%016llx\n", i, Ranges[i].BaseAddress));
            DEBUG((DEBUG_ERROR, "\t[%02d] RangeSize = 0x%016llx\tType = %s\n", i, Ranges[i].Length, CacheTypeShortNames[Ranges[i].Type]));
            DEBUG((DEBUG_ERROR, "\t[%02d] RangeEnd  = 0x%016llx\n", i, Ranges[i].BaseAddress + Ranges[i].Length - 1));
        }
    }
}

VOID
DumpEffectiveRangesCArrayStyle (
  IN MTRR_MEMORY_RANGE *EffectiveRanges,
  IN UINT32 EffectiveRangesCount
  )
{
    UINT32 i;
    if (EffectiveRanges != NULL)
    {
        for (i = 0; i < EffectiveRangesCount; i++)
        {
            DEBUG((DEBUG_ERROR, "\t{ 0x%016llx, 0x%016llx, %s },\n", EffectiveRanges[i].BaseAddress, EffectiveRanges[i].Length, CacheTypeFullNames[EffectiveRanges[i].Type]));
        }
    }
}

/*
The generated ranges must be above 1M. Because Variable MTRR are only used for >1M.

Not all overlappings are valid. The valid ones are: (Refer to: Intel SDM 11.11.4.1)
  UC: UC, WT, WB, WC, WP
  WT: UC, WB
  WB: UC, WT
  WC: UC
  WP: UC

*/
VOID
GenerateValidAndConfigurableMtrrPairs (
  IN OUT MTRR_VARIABLE_SETTING **VariableMtrrSettings,
  IN OUT MTRR_MEMORY_RANGE **RawMtrrMemoryRanges,
  IN UINT32 UcCount,
  IN UINT32 WtCount,
  IN UINT32 WbCount,
  IN UINT32 WpCount,
  IN UINT32 WcCount
  )
{
    UINT32 TotalMtrrCount = UcCount + WtCount + WbCount + WpCount + WcCount;
    UINT32 FirmwareVariableMtrrCount = GetFirmwareVariableMtrrCount();
    ASSERT(TotalMtrrCount <= FirmwareVariableMtrrCount);
    UINT32 i = 0;
    (*VariableMtrrSettings) = calloc(TotalMtrrCount, sizeof(MTRR_VARIABLE_SETTING));

    (*RawMtrrMemoryRanges) = calloc(TotalMtrrCount, sizeof(MTRR_MEMORY_RANGE));

    //1. Generate UC, WT, WB at will.
    for (i = 0; i < UcCount; i++)
    {
        ComposeMtrrPairOfSpecificType(&((*VariableMtrrSettings)[i]), CacheUncacheable, &((*RawMtrrMemoryRanges)[i]));
    }

    for (i = UcCount; i < UcCount + WtCount; i++)
    {
        ComposeMtrrPairOfSpecificType(&((*VariableMtrrSettings)[i]), CacheWriteThrough, &((*RawMtrrMemoryRanges)[i]));
    }

    for (i = UcCount + WtCount; i < UcCount + WtCount + WbCount; i++)
    {
        ComposeMtrrPairOfSpecificType(&((*VariableMtrrSettings)[i]), CacheWriteBack, &((*RawMtrrMemoryRanges)[i]));
    }

    //2. Generate WP MTRR and DO NOT overlap with WT, WB.
    for (i = UcCount + WtCount + WbCount; i < UcCount + WtCount + WbCount + WpCount; i++)
    {
        ComposeMtrrPairOfSpecificType(&((*VariableMtrrSettings)[i]), CacheWriteProtected, &((*RawMtrrMemoryRanges)[i]));
        while (WpOverlapWithWtWb(*RawMtrrMemoryRanges, UcCount, WtCount, WbCount, (*RawMtrrMemoryRanges)[i]))
        {
            ComposeMtrrPairOfSpecificType(&((*VariableMtrrSettings)[i]), CacheWriteProtected, &((*RawMtrrMemoryRanges)[i]));
        }
    }

    //3. Generate WC MTRR and DO NOT overlap with WT, WB, WP.
    for (i = UcCount + WtCount + WbCount + WpCount; i < TotalMtrrCount; i++)
    {
        ComposeMtrrPairOfSpecificType(&((*VariableMtrrSettings)[i]), CacheWriteCombining, &((*RawMtrrMemoryRanges)[i]));
        while (WcOverlapWithWtWbWp(*RawMtrrMemoryRanges, UcCount, WtCount, WbCount, WpCount, (*RawMtrrMemoryRanges)[i]))
        {
            ComposeMtrrPairOfSpecificType(&((*VariableMtrrSettings)[i]), CacheWriteCombining, &((*RawMtrrMemoryRanges)[i]));
        }
    }

    //DumpMtrrAndRawRanges(*VariableMtrrSettings, *RawMtrrMemoryRanges, TotalMtrrCount);
}

UINT32
GetRandomValidCacheType (
  VOID
  )
{
    return MtrrCacheTypeValues[rand() % 5];
}

UINT64
GenerateRandomUint64 (
  VOID
  )
{
  return ((UINT64)(rand() & 0x0fff) << 52 |
          (UINT64)(rand() & 0x0fff) << 40 |
          (UINT64)(rand() & 0x0fff) << 28 |
          (UINT64)(rand() & 0x0fff) << 16 |
          (UINT64)(rand() & 0x0fff) << 4  |
          (UINT64)(rand() & 0x000f)); // rand() only returns RAND_MAX 0x7fff according to stdlib.h. This is to generate a 64-bit random number from it.
}

VOID
GenerateInvalidMemoryLayout (
  IN OUT MTRR_MEMORY_RANGE **EffectiveMemoryRanges,
  IN UINT32 RangesCount
  )
{
    (*EffectiveMemoryRanges) = calloc(RangesCount, sizeof(MTRR_MEMORY_RANGE));
    UINT64 MaxAddressPlus1 = (((UINT64)1) << TEST_PhysicalAddressBits);

    UINT32 i = 0;
    for (i = 0; i < RangesCount; i++)
    {
        // generate random memory range type
        MTRR_MEMORY_CACHE_TYPE Type = (MTRR_MEMORY_CACHE_TYPE)(GetRandomValidCacheType());

        // generate random memory range base
        UINT64 num = GenerateRandomUint64();
        UINT64 BaseAddress = (num % MaxAddressPlus1);

        // generate random memory range length
        num = GenerateRandomUint64();
        UINT64 Length = num % (MaxAddressPlus1 - BaseAddress) + 1;

        (*EffectiveMemoryRanges)[i].BaseAddress = BaseAddress;
        (*EffectiveMemoryRanges)[i].Length = Length;
        (*EffectiveMemoryRanges)[i].Type = Type;
    }
}

INT32
CompareFuncUint64 (
  const void * a,
  const void * b
  )
{
    INT64 diff = (*(UINT64*)a - *(UINT64*)b);
    if (diff > 0)return 1;
    if (diff == 0)return 0;
    return -1;
}

VOID
DumpAllRangePiecesEndPoints (
  IN UINT64 *AllRangePiecesEndPoints,
  IN UINT32 AllRangePiecesEndPointCount
  )
{
    //DEBUG((DEBUG_ERROR, "---- All Raw Range Endpoints Sorted ----\n"));
    UINT32 i;
    for (i = 0; i < AllRangePiecesEndPointCount; i++)
        DEBUG((DEBUG_ERROR, "\t[%03d] 0x%016llx\n", i, AllRangePiecesEndPoints[i]));
}

VOID
DetermineMemoryCacheType (
  IN OUT MTRR_MEMORY_RANGE *TargetRangePiece,
  IN MTRR_MEMORY_RANGE *RawMtrrMemoryRanges,
  IN UINT32 RawMtrrMemoryRangeCount
  )
{
    UINT32 i;
    TargetRangePiece->Type = NotSpecifiedWithCurrentMtrrSetting;
    for (i = 0; i < RawMtrrMemoryRangeCount; i++)
    {
        if (RangeOverlap(TargetRangePiece, &RawMtrrMemoryRanges[i]))
        {
            if (RawMtrrMemoryRanges[i].Type < TargetRangePiece->Type)
            {
                TargetRangePiece->Type = RawMtrrMemoryRanges[i].Type;
            }
        }
    }

    if (TargetRangePiece->Type == NotSpecifiedWithCurrentMtrrSetting)
    {
        TargetRangePiece->Type = TEST_DefaultCacheType;
    }
}


/*
Get the index of the element that does NOT equals to Array[Index].
*/
UINT32
GetNextDifferentElementInSortedArray (
  IN OUT UINT64 *Array,
  IN UINT32 Index,
  IN UINT32 Count
  )
{
    UINT64 CurrentElement = Array[Index];
    while (CurrentElement == Array[Index] && Index < Count)
    {
        Index++;
    }
    return Index;
}

/*
Count: On input, the original count, on ouput, the count after removing duplicates.
*/
VOID
RemoveDuplicatesInSortedArray (
  IN OUT UINT64 *Array,
  IN OUT UINT32 *pCount
  )
{
    UINT32 i = 0;
    UINT32 NewCount = 0;

    while (i < *pCount)
    {
        Array[NewCount] = Array[i];
        NewCount++;
        i = GetNextDifferentElementInSortedArray(Array, i, *pCount);
    }
    *pCount = NewCount;
}

BOOLEAN
AddressInRange (
  IN UINT64 Address,
  IN MTRR_MEMORY_RANGE Range
  )
{
    return (Address >= Range.BaseAddress) && (Address <= Range.BaseAddress + Range.Length - 1);
}

UINT64
GetOverlapBitFlag (
  IN MTRR_MEMORY_RANGE *RawMtrrMemoryRanges,
  IN UINT32 RawMtrrMemoryRangeCount,
  IN UINT64 Address
  )
{
    UINT64 OverlapBitFlag = 0;
    UINT32 i;
    for (i = 0; i < RawMtrrMemoryRangeCount; i++)
    {
        if (AddressInRange(Address, RawMtrrMemoryRanges[i]))
        {
            OverlapBitFlag |= (((UINT64)1) << i);
        }
    }

    return OverlapBitFlag;
}

/*
return:
    0: Flag1 == Flag2
    1: Flag1 is a subset of Flag2
    2: Flag2 is a subset of Flag1
    3: No subset relations between Flag1 and Flag2.
*/
UINT32
CheckOverlapBitFlagsRelation (
  IN UINT64 Flag1,
  IN UINT64 Flag2
  )
{
    if (Flag1 == Flag2) return 0;
    if ((Flag1 | Flag2) == Flag2) return 1;
    if ((Flag1 | Flag2) == Flag1) return 2;
    return 3;
}

BOOLEAN
IsLeftSkippedEndpointAlreadyCollected (
  IN UINT64 SkippedEndpoint,
  IN MTRR_MEMORY_RANGE *AllRangePieces,
  IN UINT32 AllRangePiecesCountActual
  )
{
    UINT32 i;
    for (i = 0; i < AllRangePiecesCountActual; i++)
    {
        if (AddressInRange(SkippedEndpoint, AllRangePieces[i]))
            return TRUE;
    }
    return FALSE;
}


/*
Compact adjacent ranges of the same type.
*/
VOID
CompactAndExtendEffectiveMtrrMemoryRanges (
  IN OUT MTRR_MEMORY_RANGE **EffectiveMtrrMemoryRanges,
  IN OUT UINT32 *EffectiveMtrrMemoryRangesCount
  )
{
    UINT32 NewRangesCountAtMost = *EffectiveMtrrMemoryRangesCount + 2; // At most with 2 more range entries.
    MTRR_MEMORY_RANGE *NewRanges = (MTRR_MEMORY_RANGE*)calloc(NewRangesCountAtMost, sizeof(MTRR_MEMORY_RANGE));
    UINT32 NewRangesCountActual = 0;
    MTRR_MEMORY_RANGE *CurrentRangeInNewRanges;

    MTRR_MEMORY_RANGE *OldRanges = *EffectiveMtrrMemoryRanges;

    if (OldRanges[0].BaseAddress > 0)
    {
        NewRanges[NewRangesCountActual].BaseAddress = 0;
        NewRanges[NewRangesCountActual].Length = OldRanges[0].BaseAddress;
        NewRanges[NewRangesCountActual].Type = TEST_DefaultCacheType; // CacheUncacheable; // Default type can be UC or WB..
        NewRangesCountActual++;
    }

    UINT32 OldRangesIndex = 0;
    while (OldRangesIndex < *EffectiveMtrrMemoryRangesCount)
    {
        MTRR_MEMORY_CACHE_TYPE CurrentRangeTypeInOldRanges = OldRanges[OldRangesIndex].Type;
        CurrentRangeInNewRanges = NULL;
        if (NewRangesCountActual > 0) // We need to check CurrentNewRange first before generate a new NewRange.
        {
            CurrentRangeInNewRanges = &NewRanges[NewRangesCountActual - 1];
        }
        if (CurrentRangeInNewRanges != NULL && CurrentRangeInNewRanges->Type == CurrentRangeTypeInOldRanges)
        {
            CurrentRangeInNewRanges->Length += OldRanges[OldRangesIndex].Length;
        }
        else
        {
            NewRanges[NewRangesCountActual].BaseAddress = OldRanges[OldRangesIndex].BaseAddress;
            NewRanges[NewRangesCountActual].Length += OldRanges[OldRangesIndex].Length;
            NewRanges[NewRangesCountActual].Type = CurrentRangeTypeInOldRanges;
            while (OldRangesIndex + 1 < *EffectiveMtrrMemoryRangesCount && OldRanges[OldRangesIndex + 1].Type == CurrentRangeTypeInOldRanges) // TODO add index limit check
            {
                OldRangesIndex++;
                NewRanges[NewRangesCountActual].Length += OldRanges[OldRangesIndex].Length;
            }
            NewRangesCountActual++;
        }

        OldRangesIndex++;
    }

    UINT64 MaxAddress = ((UINT64)1 << TEST_PhysicalAddressBits) - 1;
    MTRR_MEMORY_RANGE OldLastRange = OldRanges[(*EffectiveMtrrMemoryRangesCount) - 1];
    CurrentRangeInNewRanges = &NewRanges[NewRangesCountActual - 1];
    if (OldLastRange.BaseAddress + OldLastRange.Length - 1 < MaxAddress)
    {
        if (CurrentRangeInNewRanges->Type == TEST_DefaultCacheType)
        {
            CurrentRangeInNewRanges->Length = MaxAddress - CurrentRangeInNewRanges->BaseAddress + 1;
        }
        else
        {
            NewRanges[NewRangesCountActual].BaseAddress = OldLastRange.BaseAddress + OldLastRange.Length;
            NewRanges[NewRangesCountActual].Length = MaxAddress - NewRanges[NewRangesCountActual].BaseAddress + 1;
            NewRanges[NewRangesCountActual].Type = TEST_DefaultCacheType;
            NewRangesCountActual++;
        }
    }

    free(*EffectiveMtrrMemoryRanges);
    *EffectiveMtrrMemoryRanges = NewRanges;
    *EffectiveMtrrMemoryRangesCount = NewRangesCountActual;
}

VOID
CollectRawMtrrRangesEndpointsAndSortAndRemoveDuplicates (
  IN UINT64 *AllEndPointsInclusive,
  IN UINT32 *pAllEndPointsCount,
  IN MTRR_MEMORY_RANGE *RawMtrrMemoryRanges,
  IN UINT32 RawMtrrMemoryRangesCount
  )
{
    if ((RawMtrrMemoryRangesCount << 1) != *pAllEndPointsCount)
    {
        DEBUG((DEBUG_ERROR, "EndpointCount and RawMtrrMemoryRanges mismatch.\n"));
        while (1);
    }

    UINT32 i;
    for (i = 0; i < *pAllEndPointsCount; i += 2)
    {
        UINT32 RawRangeIndex = i >> 1;
        AllEndPointsInclusive[i] = RawMtrrMemoryRanges[RawRangeIndex].BaseAddress;
        AllEndPointsInclusive[i + 1] = RawMtrrMemoryRanges[RawRangeIndex].BaseAddress + RawMtrrMemoryRanges[RawRangeIndex].Length - 1;
    }

    qsort(AllEndPointsInclusive, *pAllEndPointsCount, sizeof(UINT64), CompareFuncUint64);
    RemoveDuplicatesInSortedArray(AllEndPointsInclusive, pAllEndPointsCount);
    //DumpAllRangePiecesEndPoints(AllEndPointsInclusive, *pAllEndPointsCount);
}

VOID
GetEffectiveMemoryRanges (
  IN MTRR_MEMORY_RANGE *RawMtrrMemoryRanges,
  IN UINT32 RawMtrrMemoryRangesCount,
  OUT MTRR_MEMORY_RANGE **EffectiveMtrrMemoryRanges,
  OUT UINT32 *EffectiveMtrrMemoryRangesCount
  )
{
    UINT32 i;
    UINT32 AllEndPointsCount = RawMtrrMemoryRangesCount << 1;
    UINT64 *AllEndPointsInclusive = calloc(AllEndPointsCount, sizeof(UINT64));
    UINT32 AllRangePiecesCountMax = RawMtrrMemoryRangesCount * 3 + 1;
    MTRR_MEMORY_RANGE *AllRangePieces = calloc(AllRangePiecesCountMax, sizeof(MTRR_MEMORY_RANGE));
    UINT32 AllRangePiecesCountActual = 0;


    CollectRawMtrrRangesEndpointsAndSortAndRemoveDuplicates(AllEndPointsInclusive, &AllEndPointsCount, RawMtrrMemoryRanges, RawMtrrMemoryRangesCount);

    i = 0;
    AllRangePiecesCountActual = 0;
    UINT64 OverlapBitFlag1 = 0;
    UINT64 OverlapBitFlag2 = 0;
    INT32 OverlapFlagRelation;
    while (i < AllEndPointsCount - 1)
    {
        OverlapBitFlag1 = GetOverlapBitFlag(RawMtrrMemoryRanges, RawMtrrMemoryRangesCount, AllEndPointsInclusive[i]);
        OverlapBitFlag2 = GetOverlapBitFlag(RawMtrrMemoryRanges, RawMtrrMemoryRangesCount, AllEndPointsInclusive[i + 1]);
        OverlapFlagRelation = CheckOverlapBitFlagsRelation(OverlapBitFlag1, OverlapBitFlag2);
        switch (OverlapFlagRelation)
        {
            case 0: // [1, 2]
                AllRangePieces[AllRangePiecesCountActual].BaseAddress = AllEndPointsInclusive[i];
                AllRangePieces[AllRangePiecesCountActual].Length = AllEndPointsInclusive[i + 1] - AllEndPointsInclusive[i] + 1;
                AllRangePiecesCountActual++;
                break;
            case 1: // [1, 2)
                AllRangePieces[AllRangePiecesCountActual].BaseAddress = AllEndPointsInclusive[i];
                AllRangePieces[AllRangePiecesCountActual].Length = (AllEndPointsInclusive[i + 1] - 1) - AllEndPointsInclusive[i] + 1;
                AllRangePiecesCountActual++;
                break;
            case 2: // (1, 2]
                AllRangePieces[AllRangePiecesCountActual].BaseAddress = AllEndPointsInclusive[i] + 1;
                AllRangePieces[AllRangePiecesCountActual].Length = AllEndPointsInclusive[i + 1] - (AllEndPointsInclusive[i] + 1) + 1;
                AllRangePiecesCountActual++;

                if (!IsLeftSkippedEndpointAlreadyCollected(AllEndPointsInclusive[i], AllRangePieces, AllRangePiecesCountActual))
                {
                    AllRangePieces[AllRangePiecesCountActual].BaseAddress = AllEndPointsInclusive[i];
                    AllRangePieces[AllRangePiecesCountActual].Length = 1;
                    AllRangePiecesCountActual++;
                }
                break;
            case 3: // (1, 2)
                AllRangePieces[AllRangePiecesCountActual].BaseAddress = AllEndPointsInclusive[i] + 1;
                AllRangePieces[AllRangePiecesCountActual].Length = (AllEndPointsInclusive[i + 1] - 1) - (AllEndPointsInclusive[i] + 1) + 1;
                if (AllRangePieces[AllRangePiecesCountActual].Length == 0) // Only in case 3 can exists Length=0, we should skip such "segment".
                    break;
                AllRangePiecesCountActual++;
                if (!IsLeftSkippedEndpointAlreadyCollected(AllEndPointsInclusive[i], AllRangePieces, AllRangePiecesCountActual))
                {
                    AllRangePieces[AllRangePiecesCountActual].BaseAddress = AllEndPointsInclusive[i];
                    AllRangePieces[AllRangePiecesCountActual].Length = 1;
                    AllRangePiecesCountActual++;
                }
                break;
            default:
                ASSERT(FALSE);
        }
        i++;
    }

    for (i = 0; i < AllRangePiecesCountActual; i++)
    {
        DetermineMemoryCacheType(&AllRangePieces[i], RawMtrrMemoryRanges, RawMtrrMemoryRangesCount);
    }

    free(AllEndPointsInclusive);
    AllEndPointsInclusive = NULL;
    //*EffectiveMtrrMemoryRanges = AllRangePieces; // Remember to free this memory in caller.
    //*EffectiveMtrrMemoryRangeCount = AllRangePiecesCountActual;
    //DEBUG((DEBUG_ERROR, "----Before Compact----\n"));
    //DumpMtrrAndRawRanges(NULL, AllRangePieces, AllRangePiecesCountActual);
    CompactAndExtendEffectiveMtrrMemoryRanges(&AllRangePieces, &AllRangePiecesCountActual);
    //DEBUG((DEBUG_ERROR, "----After Compact----\n"));
    //DumpMtrrAndRawRanges(NULL, AllRangePieces, AllRangePiecesCountActual); // Only dump when failure happens.
    *EffectiveMtrrMemoryRanges = AllRangePieces; // Remember to free this memory in caller.
    *EffectiveMtrrMemoryRangesCount = AllRangePiecesCountActual;
}


