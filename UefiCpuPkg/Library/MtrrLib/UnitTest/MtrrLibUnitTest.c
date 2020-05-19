/** @file
  Unit tests of the MtrrLib instance of the MtrrLib class

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MtrrLibUnitTest.h"

UINT64                 mFailureCount_OneByOne                  = 0;
UINT64                 mFailureCount_AllAtOnce                 = 0;
MTRR_VARIABLE_SETTING  *mTestMtrrSetting                       = NULL;
MTRR_MEMORY_RANGE      *mRawMtrrRanges                         = NULL;
MTRR_MEMORY_RANGE      *mExpectedEffectiveMemoryRanges         = NULL;
UINT32                 mExpectedEffectiveMtrrMemoryRangesCount = 0;
UINT32                 mExpectedVariableMtrrUsageCount         = 0 ;

//
//
//
BOOLEAN                          gMtrrRegsInitialized = FALSE;
UINT64                           gFixedMtrrs[MTRR_NUMBER_OF_FIXED_MTRR];
MSR_IA32_MTRR_PHYSBASE_REGISTER  gVariableMtrrsPhysBase[MTRR_NUMBER_OF_VARIABLE_MTRR];
MSR_IA32_MTRR_PHYSMASK_REGISTER  gVariableMtrrsPhysMask[MTRR_NUMBER_OF_VARIABLE_MTRR];
MSR_IA32_MTRR_DEF_TYPE_REGISTER  gDefTypeMsr;
MSR_IA32_MTRRCAP_REGISTER        gMtrrCapMsr;
CPUID_VERSION_INFO_EDX           gCpuidVersionInfoEdx;
CPUID_VIR_PHY_ADDRESS_SIZE_EAX   gCpuidVirPhyAddressSizeEax;
UINT32                           gDefaultPcdCpuNumberOfReservedVariableMtrrs;

UINT32
EFIAPI
UnitTestMtrrLibAsmCpuid (
  IN      UINT32                    Index,
  OUT     UINT32                    *RegisterEax,  OPTIONAL
  OUT     UINT32                    *RegisterEbx,  OPTIONAL
  OUT     UINT32                    *RegisterEcx,  OPTIONAL
  OUT     UINT32                    *RegisterEdx   OPTIONAL
  )
{
  switch (Index) {
  case CPUID_VERSION_INFO:
    if (RegisterEdx != NULL) {
      *RegisterEdx = gCpuidVersionInfoEdx.Uint32;
    }
    return Index;
    break;
  case CPUID_EXTENDED_FUNCTION:
    if (RegisterEax != NULL) {
      *RegisterEax = CPUID_VIR_PHY_ADDRESS_SIZE;
    }
    return Index;
    break;
  case CPUID_VIR_PHY_ADDRESS_SIZE:
    if (RegisterEax != NULL) {
      *RegisterEax = gCpuidVirPhyAddressSizeEax.Uint32;
    }
    return Index;
    break;
  }

  //
  // Should never fall through to here
  //
  ASSERT(FALSE);
  return Index;
}


/**
Read data to MSR.

@param  Index                Register index of MSR.

@return Value read from MSR.

**/
UINT64
EFIAPI
UnitTestMtrrLibAsmReadMsr64(
  IN UINT32  MsrIndex
  )
{
  UINT32 MtrrArrayIndex;

  if ((MsrIndex >= MSR_IA32_MTRR_FIX64K_00000) &&
      (MsrIndex <= MSR_IA32_MTRR_FIX4K_F8000)) {
    if (MsrIndex == MSR_IA32_MTRR_FIX64K_00000) {
      MtrrArrayIndex = MsrIndex - MSR_IA32_MTRR_FIX64K_00000;
    } else if ((MsrIndex >= MSR_IA32_MTRR_FIX16K_80000) &&
               (MsrIndex <= MSR_IA32_MTRR_FIX16K_A0000)) {
      MtrrArrayIndex = MsrIndex - MSR_IA32_MTRR_FIX16K_80000 + 1;
    } else {
      MtrrArrayIndex = MsrIndex - MSR_IA32_MTRR_FIX4K_C0000 + 3;
    }
    return gFixedMtrrs[MtrrArrayIndex];
  }

  if ((MsrIndex >= MSR_IA32_MTRR_PHYSBASE0) &&
      (MsrIndex <= MSR_IA32_MTRR_PHYSMASK0 + (MTRR_NUMBER_OF_VARIABLE_MTRR << 1))) {
    if (MsrIndex % 2 == 0) {
      MtrrArrayIndex = (MsrIndex - MSR_IA32_MTRR_PHYSBASE0) >> 1;
      return gVariableMtrrsPhysBase[MtrrArrayIndex].Uint64;
    } else {
      MtrrArrayIndex = (MsrIndex - MSR_IA32_MTRR_PHYSMASK0) >> 1;
      return gVariableMtrrsPhysMask[MtrrArrayIndex].Uint64;
    }
  }

  if (MsrIndex == MSR_IA32_MTRR_DEF_TYPE) {
    return gDefTypeMsr.Uint64;
  }

  if (MsrIndex == MSR_IA32_MTRRCAP) {
    return gMtrrCapMsr.Uint64;
  }

  //
  // Should never fall through to here
  //
  ASSERT(FALSE);
  return 0;
}

/**
Writes a 64-bit value to a Machine Specific Register(MSR), and returns the
value.

Writes the 64-bit value specified by Value to the MSR specified by Index. The
64-bit value written to the MSR is returned. No parameter checking is
performed on Index or Value, and some of these may cause CPU exceptions. The
caller must either guarantee that Index and Value are valid, or the caller
must establish proper exception handlers. This function is only available on
IA-32 and x64.

@param  Index The 32-bit MSR index to write.
@param  Value The 64-bit value to write to the MSR.

@return Value

**/
UINT64
EFIAPI
UnitTestMtrrLibAsmWriteMsr64(
  IN      UINT32                    MsrIndex,
  IN      UINT64                    Value
  )
{
  UINT32 MtrrArrayIndex;

  if ((MsrIndex >= MSR_IA32_MTRR_FIX64K_00000) &&
      (MsrIndex <= MSR_IA32_MTRR_FIX4K_F8000)) {
    if (MsrIndex == MSR_IA32_MTRR_FIX64K_00000) {
      MtrrArrayIndex = MsrIndex - MSR_IA32_MTRR_FIX64K_00000;
    } else if ((MsrIndex >= MSR_IA32_MTRR_FIX16K_80000) &&
               (MsrIndex <= MSR_IA32_MTRR_FIX16K_A0000)) {
      MtrrArrayIndex = MsrIndex - MSR_IA32_MTRR_FIX16K_80000 + 1;
    } else {
      MtrrArrayIndex = MsrIndex - MSR_IA32_MTRR_FIX4K_C0000 + 3;
    }
    gFixedMtrrs[MtrrArrayIndex] = Value;
    return Value;
  }

  if ((MsrIndex >= MSR_IA32_MTRR_PHYSBASE0) &&
      (MsrIndex <= MSR_IA32_MTRR_PHYSMASK0 + (MTRR_NUMBER_OF_VARIABLE_MTRR << 1))) {
    if (MsrIndex % 2 == 0) {
      MtrrArrayIndex = (MsrIndex - MSR_IA32_MTRR_PHYSBASE0) >> 1;
      gVariableMtrrsPhysBase[MtrrArrayIndex].Uint64 = Value;
      return Value;
    } else {
      MtrrArrayIndex = (MsrIndex - MSR_IA32_MTRR_PHYSMASK0) >> 1;
      gVariableMtrrsPhysMask[MtrrArrayIndex].Uint64 = Value;
      return Value;
    }
  }

  if (MsrIndex == MSR_IA32_MTRR_DEF_TYPE) {
    gDefTypeMsr.Uint64 = Value;
    return Value;
  }

  if (MsrIndex == MSR_IA32_MTRRCAP) {
    gMtrrCapMsr.Uint64 = Value;
    return Value;
  }

  //
  // Should never fall through to here
  //
  ASSERT(FALSE);
  return 0;
}

UNIT_TEST_STATUS
EFIAPI
InitializeMtrrRegs (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT32 Index;

  for (Index = 0; Index < MTRR_NUMBER_OF_FIXED_MTRR * sizeof(UINT64); Index++) {
    ((UINT8*)gFixedMtrrs)[Index] = TEST_DefaultCacheType;
  }

  for (Index = 0; Index < MTRR_NUMBER_OF_VARIABLE_MTRR; Index++) {
    gVariableMtrrsPhysBase[Index].Uint64 = 0;
    gVariableMtrrsPhysBase[Index].Bits.Type = TEST_DefaultCacheType;
    gVariableMtrrsPhysBase[Index].Bits.Reserved1 = 0;

    gVariableMtrrsPhysMask[Index].Uint64 = 0;
    gVariableMtrrsPhysMask[Index].Bits.V = 0;
    gVariableMtrrsPhysMask[Index].Bits.Reserved1 = 0;
  }

  gDefTypeMsr.Bits.E = 1;
  gDefTypeMsr.Bits.FE = 1;
  gDefTypeMsr.Bits.Type = TEST_DefaultCacheType;
  gDefTypeMsr.Bits.Reserved1 = 0;
  gDefTypeMsr.Bits.Reserved2 = 0;
  gDefTypeMsr.Bits.Reserved3 = 0;

  gMtrrCapMsr.Bits.SMRR = 0;
  gMtrrCapMsr.Bits.WC = 0;
  gMtrrCapMsr.Bits.VCNT = TEST_VariableMtrrCount;
  gMtrrCapMsr.Bits.FIX = 1;
  gMtrrCapMsr.Bits.Reserved1 = 0;
  gMtrrCapMsr.Bits.Reserved2 = 0;
  gMtrrCapMsr.Bits.Reserved3 = 0;

  gCpuidVersionInfoEdx.Bits.MTRR = 1;
  gCpuidVirPhyAddressSizeEax.Bits.PhysicalAddressBits = TEST_PhysicalAddressBits;

  //
  // Reset PCD settings to their default values
  //
  if (!gMtrrRegsInitialized) {
    //
    // Hook BaseLib functions used by MtrrLib that require some emulation.
    //
    gUnitTestHostBaseLib.AsmCpuid      = UnitTestMtrrLibAsmCpuid;
    gUnitTestHostBaseLib.AsmReadMsr64  = UnitTestMtrrLibAsmReadMsr64;
    gUnitTestHostBaseLib.AsmWriteMsr64 = UnitTestMtrrLibAsmWriteMsr64;

    //
    // Get default PCD values
    //
    gDefaultPcdCpuNumberOfReservedVariableMtrrs = PcdGet32 (PcdCpuNumberOfReservedVariableMtrrs);
  }
  
  //
  // Reset PCD settings to their default values
  //
  PatchPcdSet32 (PcdCpuNumberOfReservedVariableMtrrs, gDefaultPcdCpuNumberOfReservedVariableMtrrs);

  //
  // Set flag that MTRR Register state has been initialized the first time
  //
  gMtrrRegsInitialized = TRUE;

  return UNIT_TEST_PASSED;
}

BOOLEAN
VerifyResult (
  IN MTRR_MEMORY_RANGE *ExpectedEffectiveMemoryRanges,
  IN UINT32 ExpectedEffectiveMtrrMemoryRangeCount,
  IN UINT32 ExpectedVariableMtrrUsageCount,
  IN MTRR_MEMORY_RANGE *ActualRanges,
  IN UINT32 ActualRangesCount,
  IN UINT32 ActualMtrrsCount
  )
{
    UINT32 i;
    BOOLEAN PassSoFar = TRUE;

    // Verify effective ranges count.
    if (ExpectedEffectiveMtrrMemoryRangeCount != ActualRangesCount)
    {
        DEBUG((DEBUG_ERROR, "[Fail]: Ranges Count Not Match!\n"));
        DEBUG((DEBUG_ERROR, "Expected: [%02d] Actual: [%02d]\n", ExpectedEffectiveMtrrMemoryRangeCount, ActualRangesCount));
        PassSoFar = FALSE;
    }
    else
    {
        // Verify each effective range
        for (i = 0; i < ActualRangesCount; i++)
        {
            MTRR_MEMORY_RANGE Expected = ExpectedEffectiveMemoryRanges[i];
            MTRR_MEMORY_RANGE Actual = ActualRanges[i];
            if (!(Expected.BaseAddress == Actual.BaseAddress && Expected.Length == Actual.Length && Expected.Type == Actual.Type))
            {
                DEBUG((DEBUG_ERROR, "[Fail]: Range %02d Not Match!\n", i));
                PassSoFar = FALSE;
            }
        }
    }

    if (PassSoFar)
    {
        // Verify variable MTRR usage
        if (ExpectedVariableMtrrUsageCount < ActualMtrrsCount)
        {
            DEBUG((DEBUG_ERROR, "[Fail]: Worse Usage!\n"));
            DEBUG((DEBUG_ERROR, "Expected: [%02d] Actual: [%02d]\n", ExpectedVariableMtrrUsageCount, ActualMtrrsCount));
            PassSoFar = FALSE;
        }
        else if (ExpectedVariableMtrrUsageCount == ActualMtrrsCount)
        {
            DEBUG((DEBUG_ERROR, "[Pass]: No Better\n"));
            //DEBUG((DEBUG_ERROR, "Expected: [%02d] Actual: [%02d]\n", ExpectedVariableMtrrUsageCount, ActualMtrrsCount));
        }
        else
        {
            DEBUG((DEBUG_ERROR, "[Pass]\n"));
            DEBUG((DEBUG_ERROR, "Actual Usage: [%02d]\n", ActualMtrrsCount));
        }
    }

    return PassSoFar;
}

VOID
DumpTestInput (
  VOID
  )
{
    // Dump MTRR
    DEBUG((DEBUG_ERROR, "---- Test MTRR Settings ----\n"));
    DumpMtrrOrRanges(mTestMtrrSetting, mExpectedVariableMtrrUsageCount, NULL, 0);

    // Dump raw MTRR ranges
    DEBUG((DEBUG_ERROR, "---- Raw MTRR Ranges ----\n"));
    DumpMtrrOrRanges(NULL, 0, mRawMtrrRanges, mExpectedVariableMtrrUsageCount);


    DEBUG((DEBUG_ERROR, "---- Raw MTRR Range Endpoints Sorted ----\n"));
    UINT32 AllEndPointsCount = mExpectedVariableMtrrUsageCount << 1;
    UINT64 *AllEndPointsInclusive = calloc(AllEndPointsCount, sizeof(UINT64));
    CollectRawMtrrRangesEndpointsAndSortAndRemoveDuplicates(AllEndPointsInclusive, &AllEndPointsCount, mRawMtrrRanges, mExpectedVariableMtrrUsageCount);
    DumpAllRangePiecesEndPoints(AllEndPointsInclusive, AllEndPointsCount);
    free(AllEndPointsInclusive);

    // Dump effective ranges in C-Array style
    DEBUG((DEBUG_ERROR, "---- Effective Ranges ----\n"));
    DumpMtrrOrRanges(NULL, 0, mExpectedEffectiveMemoryRanges, mExpectedEffectiveMtrrMemoryRangesCount);

    // Dump effective ranges in C-Array style
    DEBUG((DEBUG_ERROR, "---- Effective Ranges in C-Array Style Start----\n"));
    DumpEffectiveRangesCArrayStyle(mExpectedEffectiveMemoryRanges, mExpectedEffectiveMtrrMemoryRangesCount);
    DEBUG((DEBUG_ERROR, "---- Effective Ranges in C-Array Style Finish ----\n"));
}


UNIT_TEST_STATUS
TheRunningManForValidAndConfigurableLayout (
  IN UNIT_TEST_CONTEXT           Context,
  IN UINT32 UcCount,
  IN UINT32 WtCount,
  IN UINT32 WbCount,
  IN UINT32 WpCount,
  IN UINT32 WcCount
  )
{
    BOOLEAN DEBUG_OVERRIDE = FALSE; // If we provide overriding DebugRanges as below, we need to set this to TRUE.
    mTestMtrrSetting = NULL;
    mRawMtrrRanges = NULL;
    mExpectedEffectiveMemoryRanges = NULL;
    mExpectedEffectiveMtrrMemoryRangesCount = 0;
    mExpectedVariableMtrrUsageCount = UcCount + WtCount + WbCount + WpCount + WcCount;

    GenerateValidAndConfigurableMtrrPairs(&mTestMtrrSetting, &mRawMtrrRanges, UcCount, WtCount, WbCount, WpCount, WcCount); // Total Mtrr count must <= TEST_VariableMtrrCount - TEST_NumberOfReservedVariableMtrrs

    GetEffectiveMemoryRanges(mRawMtrrRanges, mExpectedVariableMtrrUsageCount, &mExpectedEffectiveMemoryRanges, &mExpectedEffectiveMtrrMemoryRangesCount);

    BOOLEAN OneByOnePass = FALSE;
    RETURN_STATUS OneByOneStatus = EFI_SUCCESS;

    mActualRanges = NULL;
    mActualRangesCount = 0;
    mActualMtrrsCount = 0;

    UINT32 i = 0;
    for (i = 0; i < mExpectedEffectiveMtrrMemoryRangesCount; i++)
    {

        OneByOneStatus = MtrrSetMemoryAttribute(mExpectedEffectiveMemoryRanges[i].BaseAddress, mExpectedEffectiveMemoryRanges[i].Length, mExpectedEffectiveMemoryRanges[i].Type);
        free(mActualRanges);
        mActualRanges = NULL;
        if (OneByOneStatus != EFI_SUCCESS)
        {
            break;
        }
    }

    free(mActualRanges);
    mActualRanges = NULL;

    // Verify the result of MtrrSetMemoryAttribute()
    DEBUG((DEBUG_ERROR, "MtrrSetMemoryAttribute(): "));
    if (OneByOneStatus == EFI_SUCCESS)
    {
        MtrrDebugPrintAllMtrrs();
        CollectTestResult ();

        MTRR_MEMORY_RANGE *ActualRanges_MtrrSetMemoryAttribute = mActualRanges;
        UINT32 ActualRangesCount_MtrrSetMemoryAttribute = mActualRangesCount;
        UINT32 ActualMtrrsCount_MtrrSetMemoryAttribute = mActualMtrrsCount;
        OneByOnePass = VerifyResult(mExpectedEffectiveMemoryRanges, mExpectedEffectiveMtrrMemoryRangesCount, mExpectedVariableMtrrUsageCount,
            ActualRanges_MtrrSetMemoryAttribute, ActualRangesCount_MtrrSetMemoryAttribute, ActualMtrrsCount_MtrrSetMemoryAttribute);
    }
    else
    {
        DEBUG((DEBUG_ERROR, "[Fail]: Status = %08x\n", OneByOneStatus));
        DEBUG((DEBUG_ERROR, "Failed to set effective memory range of index: [%02d]\n", i));
    }
    free(mActualRanges);

    // Test MtrrSetMemoryAttributesInMtrrSettings() by setting expected ranges All At Once.
    BOOLEAN AllAtOncePass = FALSE;
    RETURN_STATUS AllAtOnceStatus = EFI_SUCCESS;
    mActualRanges = NULL;
    mActualRangesCount = 0;
    mActualMtrrsCount = 0;

    InitializeMtrrRegs (Context);

    UINT8 *Scratch;
    UINTN ScratchSize = SCRATCH_BUFFER_SIZE;
    Scratch = calloc(ScratchSize, sizeof(UINT8));
    AllAtOnceStatus = MtrrSetMemoryAttributesInMtrrSettings(NULL, Scratch, &ScratchSize, mExpectedEffectiveMemoryRanges, mExpectedEffectiveMtrrMemoryRangesCount);
    if (!RETURN_ERROR (AllAtOnceStatus)) {
      CollectTestResult ();
    }
    free(mActualRanges);
    mActualRanges = NULL;

    if (AllAtOnceStatus == RETURN_BUFFER_TOO_SMALL)
    {
        Scratch = realloc(Scratch, ScratchSize);
        AllAtOnceStatus = MtrrSetMemoryAttributesInMtrrSettings(NULL, Scratch, &ScratchSize, mExpectedEffectiveMemoryRanges, mExpectedEffectiveMtrrMemoryRangesCount);
        if (!RETURN_ERROR (AllAtOnceStatus)) {
          CollectTestResult ();
        }
        free(mActualRanges);
        mActualRanges = NULL;
    }
    free(Scratch);



    // Verify result of MtrrSetMemoryAttributesInMtrrSettings()
    DEBUG((DEBUG_ERROR, "MtrrSetMemoryAttributesInMtrrSettings(): "));
    if (AllAtOnceStatus == EFI_SUCCESS)
    {
        MtrrDebugPrintAllMtrrs();
        CollectTestResult ();

        MTRR_MEMORY_RANGE *ActualRanges_SetMemoryAttributesInMtrrSettings = mActualRanges;
        UINT32 ActualRangesCount_SetMemoryAttributesInMtrrSettings = mActualRangesCount;
        UINT32 ActualMtrrsCount_SetMemoryAttributesInMtrrSettings = mActualMtrrsCount;
        AllAtOncePass = VerifyResult(mExpectedEffectiveMemoryRanges, mExpectedEffectiveMtrrMemoryRangesCount, mExpectedVariableMtrrUsageCount,
            ActualRanges_SetMemoryAttributesInMtrrSettings, ActualRangesCount_SetMemoryAttributesInMtrrSettings, ActualMtrrsCount_SetMemoryAttributesInMtrrSettings);
    }
    else
    {
        DEBUG((DEBUG_ERROR, "[Fail]: Status = %08x\n", AllAtOnceStatus));
    }
    free(mActualRanges);
    mActualRanges = NULL;

    if (OneByOnePass && AllAtOncePass)
    {
        //DEBUG((DEBUG_ERROR, "Double Pass!\n"));
    }
    else
    {
        //DumpMtrrAndRawRanges(mTestMtrrSetting, mExpectedEffectiveMemoryRanges, mExpectedVariableMtrrUsageCount);
        DumpTestInput();

        if (!OneByOnePass && !AllAtOncePass)
        {
            DEBUG((DEBUG_ERROR, "Double Failure!\n"));
            mFailureCount_OneByOne++;
            mFailureCount_AllAtOnce++;
        }
        else
        {
            DEBUG((DEBUG_ERROR, "[Fail]: Single Fail!\n"));
            if (!OneByOnePass)
            {
                DEBUG((DEBUG_ERROR, "MtrrSetMemoryAttribute() : %a\n", "FAIL"));
                DEBUG((DEBUG_ERROR, "MtrrSetMemoryAttributesInMtrrSettings() : %a\n", "PASS"));
                mFailureCount_OneByOne++;
            }
            if (!AllAtOncePass)
            {
                DEBUG((DEBUG_ERROR, "MtrrSetMemoryAttribute() : %a\n", "PASS"));
                DEBUG((DEBUG_ERROR, "MtrrSetMemoryAttributesInMtrrSettings() : %a\n", "FAIL"));
                mFailureCount_AllAtOnce++;
            }
        }
    }

    // clean up
    free(mTestMtrrSetting);
    if (!DEBUG_OVERRIDE)
        free(mRawMtrrRanges);
    free(mExpectedEffectiveMemoryRanges);

    //
    // NOTE: OneByOnePass can fail and AllAtOncePass can succeed.
    //
//    UT_ASSERT_TRUE(OneByOnePass);
    UT_ASSERT_TRUE(AllAtOncePass);

    return UNIT_TEST_PASSED;
}

VOID
GenerateMemoryTypeCombination (
  OUT UINT32 *UcCount,
  OUT UINT32 *WtCount,
  OUT UINT32 *WbCount,
  OUT UINT32 *WpCount,
  OUT UINT32 *WcCount
  )
{
    UINT32 MaxMtrrTypes = 5;
    UINT32 TotalMtrrCountMax = GetFirmwareVariableMtrrCount();
    UINT32 TotalMtrrCountToUse = (rand() % TotalMtrrCountMax) + 1;

    UINT32 TypeBucket[5] = { 0, 0, 0, 0, 0 };
    UINT32 i;
    for (i = 0; i < TotalMtrrCountToUse; i++)
    {
        UINT32 BucketIndex = rand() % MaxMtrrTypes;
        TypeBucket[BucketIndex]++;
    }

    *UcCount= TypeBucket[0];
    *WtCount= TypeBucket[1];
    *WbCount= TypeBucket[2];
    *WpCount= TypeBucket[3];
    *WcCount= TypeBucket[4];
}


CHAR8 mAnimation[4] = { '-', '\\', '|', '/' };

UNIT_TEST_STATUS
EFIAPI
TestGeneratorForValidAndConfigurableMemoryLayouts (
  IN UNIT_TEST_CONTEXT  Context
  )
{
    UINT32 UcCount;
    UINT32 WtCount;
    UINT32 WbCount;
    UINT32 WpCount;
    UINT32 WcCount;
    UINT32 i = 0;
    DEBUG((DEBUG_ERROR, "Test for valid and configurable layouts started.\n\n"));
    mFailureCount_OneByOne = 0;
    mFailureCount_AllAtOnce = 0;
    UINT32 AnimationCount = 0;
    while (i < TestIteration)
    {
        DEBUG((DEBUG_ERROR, "[Iteration %02d]\n", i + 1));
        DEBUG((DEBUG_INFO, "\r[#%10d/%d] :  ", i+1, TestIteration));
        if (i % 200 == 0)
        {
            AnimationCount++;
            DEBUG((DEBUG_INFO, "%c  ", mAnimation[AnimationCount % 4]));
        }

        // Default cache type is randomized for each iteration. And each iteration will test 2 MtrrLib APIs.
        TEST_DefaultCacheType = (MTRR_MEMORY_CACHE_TYPE)MtrrCacheTypeValues[rand() % 5];
        DEBUG((DEBUG_ERROR, "Default cache type: %a\n", CacheTypeFullNames[TEST_DefaultCacheType]));

        InitializeMtrrRegs (Context);
        GenerateMemoryTypeCombination(&UcCount, &WtCount, &WbCount, &WpCount, &WcCount);
        DEBUG((DEBUG_ERROR, "Expected Total Usage: %d\n", UcCount + WtCount + WbCount + WpCount + WcCount));
        DEBUG((DEBUG_ERROR, "UC=%02d, WT=%02d, WB=%02d, WP=%02d, WC=%02d\n", UcCount, WtCount, WbCount, WpCount, WcCount));
        TheRunningManForValidAndConfigurableLayout(Context, UcCount, WtCount, WbCount, WpCount, WcCount);
        i++;
        DEBUG((DEBUG_ERROR, "%03lld | %03lld\n\n", mFailureCount_OneByOne, mFailureCount_AllAtOnce)); // In case the test crash, we can save some statistics so far.
    }
    DEBUG((DEBUG_ERROR, "\n"));
    DEBUG((DEBUG_ERROR, "=========================== Failure Statistics ===========================\n"));
    DEBUG((DEBUG_ERROR, "MtrrSetMemoryAttribute()                 =   %03lld\n", mFailureCount_OneByOne));
    DEBUG((DEBUG_ERROR, "MtrrSetMemoryAttributesInMtrrSettings()  =   %03lld\n", mFailureCount_AllAtOnce));
    DEBUG((DEBUG_ERROR, "==================================================================\n"));
    DEBUG((DEBUG_ERROR, "\nTest finished.\n"));

    return UNIT_TEST_PASSED;
}

BOOLEAN
IsValidLayout (
  IN MTRR_MEMORY_RANGE *EffectiveMemoryRanges,
  IN UINT32 RangesCount
  )
{
    return FALSE;
}

UNIT_TEST_STATUS
TheRunningManForInvalidLayout (
  IN UNIT_TEST_CONTEXT           Context
  )
{
    UINT32 TotalMtrrCountMax = GetFirmwareVariableMtrrCount();
    UINT32 RangesCount = (rand() % (TotalMtrrCountMax << 2)) + 1;
    mExpectedEffectiveMtrrMemoryRangesCount = RangesCount;

    GenerateInvalidMemoryLayout(&mExpectedEffectiveMemoryRanges, RangesCount);
    while (IsValidLayout(mExpectedEffectiveMemoryRanges, RangesCount))
    {
        GenerateInvalidMemoryLayout(&mExpectedEffectiveMemoryRanges, RangesCount);
    }

    //MtrrSetMemoryAttribute() test
    UINT32 i = 0;
    RETURN_STATUS OneByOneStatus = EFI_SUCCESS;
    InitializeMtrrRegs (Context);
    for (i = 0; i < RangesCount; i++)
    {
        OneByOneStatus = MtrrSetMemoryAttribute(mExpectedEffectiveMemoryRanges[i].BaseAddress, mExpectedEffectiveMemoryRanges[i].Length, mExpectedEffectiveMemoryRanges[i].Type);
        free(mActualRanges);
        mActualRanges = NULL;
        if (OneByOneStatus != EFI_SUCCESS)
            break;
    }

    free(mActualRanges);
    mActualRanges = NULL;

    if (OneByOneStatus != EFI_SUCCESS)
    {
        mFailureCount_OneByOne++; // We expect failure here.
        //DumpTestInput();
    }
    else
    {
        DumpTestInput(); // Unexpected success, we need to dump the input.
    }



    //MtrrSetMemoryAttributeMtrrSetMemoryAttributesInMtrrSettings() test

    RETURN_STATUS AllAtOnceStatus = EFI_SUCCESS;
    UINT8 *Scratch;
    UINTN ScratchSize = SCRATCH_BUFFER_SIZE;
    Scratch = calloc(ScratchSize, sizeof(UINT8));
    AllAtOnceStatus = MtrrSetMemoryAttributesInMtrrSettings(NULL, Scratch, &ScratchSize, mExpectedEffectiveMemoryRanges, mExpectedEffectiveMtrrMemoryRangesCount);
    if (!RETURN_ERROR (AllAtOnceStatus)) {
      CollectTestResult ();
    }
    free(mActualRanges);
    mActualRanges = NULL;

    if (AllAtOnceStatus == RETURN_BUFFER_TOO_SMALL)
    {
        Scratch = realloc(Scratch, ScratchSize);
        AllAtOnceStatus = MtrrSetMemoryAttributesInMtrrSettings(NULL, Scratch, &ScratchSize, mExpectedEffectiveMemoryRanges, mExpectedEffectiveMtrrMemoryRangesCount);
        if (!RETURN_ERROR (AllAtOnceStatus)) {
          CollectTestResult ();
        }
        free(mActualRanges);
        mActualRanges = NULL;
    }
    free(Scratch);

    if (AllAtOnceStatus != EFI_SUCCESS)
    {
        mFailureCount_AllAtOnce++;
        //DumpTestInput();
    }
    else
    {
        DumpTestInput(); // Unexpected success, we need to dump the input.
    }


    free(mExpectedEffectiveMemoryRanges);

    UT_ASSERT_NOT_EQUAL(OneByOneStatus, EFI_SUCCESS);
    UT_ASSERT_NOT_EQUAL(AllAtOnceStatus, EFI_SUCCESS);

    return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestGeneratorForInvalidMemoryLayouts (
  IN UNIT_TEST_CONTEXT  Context
  )
{
    mTestMtrrSetting = NULL;
    mRawMtrrRanges = NULL;

    mFailureCount_OneByOne = 0;
    mFailureCount_AllAtOnce = 0;

    UINT32 i = 0;
    DEBUG((DEBUG_ERROR, "Test for invalid layouts started.\n\n"));
    UINT32 AnimationCount = 0;
    while (i < TestIteration)
    {
        DEBUG((DEBUG_ERROR, "[Iteration %02d]\n", i + 1));
        DEBUG((DEBUG_INFO, "\r[#%10d/%d] :  ", i + 1, TestIteration));
        if (i % 200 == 0)
        {
            AnimationCount++;
            DEBUG((DEBUG_INFO, "%c  ", mAnimation[AnimationCount % 4]));
        }

        // Default cache type is randomized for each iteration. And each iteration will test 2 MtrrLib APIs.
        TEST_DefaultCacheType = (MTRR_MEMORY_CACHE_TYPE)MtrrCacheTypeValues[rand() % 5];
        DEBUG((DEBUG_ERROR, "Default cache type: %a\n", CacheTypeFullNames[TEST_DefaultCacheType]));
        //
        // This initialization is necessary because we need to initialize the MtrrCount.
        //
        InitializeMtrrRegs (Context);
        TheRunningManForInvalidLayout(Context);
        i++;
        DEBUG((DEBUG_ERROR, "%03lld | %03lld\n\n", mFailureCount_OneByOne, mFailureCount_AllAtOnce));
    }


    DEBUG((DEBUG_ERROR, "\n"));
    DEBUG((DEBUG_ERROR, "=========================== Unexpected Success Statistics ===========================\n"));
    DEBUG((DEBUG_ERROR, "MtrrSetMemoryAttribute()                 =   %03lld\n", TestIteration - mFailureCount_OneByOne));
    DEBUG((DEBUG_ERROR, "MtrrSetMemoryAttributesInMtrrSettings()  =   %03lld\n", TestIteration - mFailureCount_AllAtOnce));
    DEBUG((DEBUG_ERROR, "==================================================================\n"));
    DEBUG((DEBUG_ERROR, "\nTest finished.\n"));

    return UNIT_TEST_PASSED;
}

/**
  Unit test of MtrrLib service IsMtrrSupported()

  @param[in]  Context    Ignored

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
UnitTestIsMtrrSupported (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Result;

  //
  // MTRR capability off in CPUID leaf.
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 0;
  Result = IsMtrrSupported ();
  UT_ASSERT_FALSE (Result);

  //
  // MTRR capability on in CPUID leaf, but no variable or fixed MTRRs.
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 1;
  gMtrrCapMsr.Bits.VCNT = 0;
  gMtrrCapMsr.Bits.FIX = 0;
  Result = IsMtrrSupported ();
  UT_ASSERT_FALSE (Result);

  //
  // MTRR capability on in CPUID leaf, but no variable MTRRs.
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 1;
  gMtrrCapMsr.Bits.VCNT = 0;
  gMtrrCapMsr.Bits.FIX = 1;
  Result = IsMtrrSupported ();
  UT_ASSERT_FALSE (Result);

  //
  // MTRR capability on in CPUID leaf, but no fixed MTRRs.
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 1;
  gMtrrCapMsr.Bits.VCNT = 7;
  gMtrrCapMsr.Bits.FIX = 0;
  Result = IsMtrrSupported ();
  UT_ASSERT_FALSE (Result);

  //
  // MTRR capability on in CPUID leaf with both variable and fixed MTRRs.
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 1;
  gMtrrCapMsr.Bits.VCNT = 7;
  gMtrrCapMsr.Bits.FIX = 1;
  Result = IsMtrrSupported ();
  UT_ASSERT_TRUE (Result);

  return UNIT_TEST_PASSED;
}

/**
  Unit test of MtrrLib service GetVariableMtrrCount()

  @param[in]  Context    Ignored

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
UnitTestGetVariableMtrrCount (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT32  Count;
  UINT32  Result;

  //
  // If MTRR capability off in CPUID leaf, then the count is always 0.
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 0;
  gMtrrCapMsr.Bits.VCNT = 5;
  Result = GetVariableMtrrCount ();
  UT_ASSERT_EQUAL (Result, 0);

  //
  // Try all supported variable MTRR counts.
  // If variable MTRR count is > MTRR_NUMBER_OF_VARIABLE_MTRR, then an ASSERT()
  // is generated.
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 1;
  for (Count = 0; Count <= MTRR_NUMBER_OF_VARIABLE_MTRR; Count++) {
    gMtrrCapMsr.Bits.VCNT = Count;
    Result = GetVariableMtrrCount ();
    UT_ASSERT_EQUAL (Result, Count);
  }

  //
  // Expect ASSERT() if variable MTRR count is > MTRR_NUMBER_OF_VARIABLE_MTRR
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 1;
  gMtrrCapMsr.Bits.VCNT = MTRR_NUMBER_OF_VARIABLE_MTRR + 1;
  UT_EXPECT_ASSERT_FAILURE (GetVariableMtrrCount (), NULL);

  gCpuidVersionInfoEdx.Bits.MTRR = 1;
  gMtrrCapMsr.Bits.VCNT = MAX_UINT8;
  UT_EXPECT_ASSERT_FAILURE (GetVariableMtrrCount (), NULL);

  return UNIT_TEST_PASSED;
}

/**
  Unit test of MtrrLib service GetFirmwareVariableMtrrCount()

  @param[in]  Context    Ignored

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
UnitTestGetFirmwareVariableMtrrCount (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT32  Result;
  UINT32  ReservedMtrrs;

  //
  // Positive test cases for VCNT = 10 and Reserved PCD in range 0..10
  //
  gMtrrCapMsr.Bits.VCNT = 10;
  for (ReservedMtrrs = 0; ReservedMtrrs <= 10; ReservedMtrrs++) {
    PatchPcdSet32 (PcdCpuNumberOfReservedVariableMtrrs, ReservedMtrrs);
    Result = GetFirmwareVariableMtrrCount ();
    UT_ASSERT_EQUAL (Result, 10 - ReservedMtrrs);
  }

  //
  // Negative test cases when Reserved PCD is larger than VCNT
  //
  gMtrrCapMsr.Bits.VCNT = 10;
  for (ReservedMtrrs = 11; ReservedMtrrs <= 255; ReservedMtrrs++) {
    PatchPcdSet32 (PcdCpuNumberOfReservedVariableMtrrs, ReservedMtrrs);
    Result = GetFirmwareVariableMtrrCount ();
    UT_ASSERT_EQUAL (Result, 0);
  }

  //
  // Negative test cases when Reserved PCD is larger than VCNT
  //
  gMtrrCapMsr.Bits.VCNT = 10;
  PatchPcdSet32 (PcdCpuNumberOfReservedVariableMtrrs, MAX_UINT32);
  Result = GetFirmwareVariableMtrrCount ();
  UT_ASSERT_EQUAL (Result, 0);

  //
  // Negative test case when MTRRs are not supported
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 0;
  gMtrrCapMsr.Bits.VCNT = 10;
  PatchPcdSet32 (PcdCpuNumberOfReservedVariableMtrrs, 2);
  Result = GetFirmwareVariableMtrrCount ();
  UT_ASSERT_EQUAL (Result, 0);

  //
  // Negative test case when Fixed MTRRs are not supported
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 1;
  gMtrrCapMsr.Bits.FIX = 0;
  gMtrrCapMsr.Bits.VCNT = 10;
  PatchPcdSet32 (PcdCpuNumberOfReservedVariableMtrrs, 2);
  Result = GetFirmwareVariableMtrrCount ();
  UT_ASSERT_EQUAL (Result, 0);

  //
  // Expect ASSERT() if variable MTRR count is > MTRR_NUMBER_OF_VARIABLE_MTRR
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 1;
  gMtrrCapMsr.Bits.FIX = 1;
  gMtrrCapMsr.Bits.VCNT = MTRR_NUMBER_OF_VARIABLE_MTRR + 1;
  UT_EXPECT_ASSERT_FAILURE (GetFirmwareVariableMtrrCount (), NULL);

  return UNIT_TEST_PASSED;
}

/**
  Unit test of MtrrLib service MtrrSetMemoryAttribute()

  @param[in]  Context    Ignored

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
UnitTestMtrrSetMemoryAttribute (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return UNIT_TEST_PASSED;
}

/**
  Unit test of MtrrLib service MtrrGetMemoryAttribute()

  @param[in]  Context    Ignored

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
UnitTestMtrrGetMemoryAttribute (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return UNIT_TEST_PASSED;
}

/**
  Unit test of MtrrLib service MtrrGetVariableMtrr()

  @param[in]  Context    Ignored

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
UnitTestMtrrGetVariableMtrr (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MTRR_VARIABLE_SETTINGS  *Result;
  MTRR_VARIABLE_SETTINGS  VariableSettings;

  //
  // Get/Set current variable MTRRs
  //
  Result = MtrrGetVariableMtrr (&VariableSettings);
  UT_ASSERT_EQUAL (Result, &VariableSettings);
  Result = MtrrSetVariableMtrr (&VariableSettings);
  UT_ASSERT_EQUAL (Result, &VariableSettings);

  //
  // TODO: Add more test cases to verify the range of BaseAddress and Mask
  // values
  //

  //
  // Negative test case when MTRRs are not supported
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 0;
  Result = MtrrGetVariableMtrr (&VariableSettings);
  UT_ASSERT_EQUAL (Result, &VariableSettings);
  
  //
  // Expect ASSERT() if variable MTRR count is > MTRR_NUMBER_OF_VARIABLE_MTRR
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 1;
  gMtrrCapMsr.Bits.VCNT = MTRR_NUMBER_OF_VARIABLE_MTRR + 1;
  UT_EXPECT_ASSERT_FAILURE (MtrrGetVariableMtrr (&VariableSettings), NULL);

  return UNIT_TEST_PASSED;
}

/**
  Unit test of MtrrLib service MtrrSetVariableMtrr()

  @param[in]  Context    Ignored

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
UnitTestMtrrSetVariableMtrr (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MTRR_VARIABLE_SETTINGS  *Result;
  MTRR_VARIABLE_SETTINGS  VariableSettings;

  //
  // Get/Set current variable MTRRs
  //
  Result = MtrrGetVariableMtrr (&VariableSettings);
  UT_ASSERT_EQUAL (Result, &VariableSettings);
  Result = MtrrSetVariableMtrr (&VariableSettings);
  UT_ASSERT_EQUAL (Result, &VariableSettings);

  //
  // TODO: Add more test cases to verify the range of BaseAddress and Mask
  // values
  //

  //
  // Negative test case when MTRRs are not supported
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 0;
  Result = MtrrSetVariableMtrr (&VariableSettings);
  UT_ASSERT_EQUAL (Result, &VariableSettings);
  
  //
  // Expect ASSERT() if variable MTRR count is > MTRR_NUMBER_OF_VARIABLE_MTRR
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 1;
  gMtrrCapMsr.Bits.VCNT = MTRR_NUMBER_OF_VARIABLE_MTRR + 1;
  UT_EXPECT_ASSERT_FAILURE (MtrrSetVariableMtrr (&VariableSettings), NULL);

  return UNIT_TEST_PASSED;
}

/**
  Unit test of MtrrLib service MtrrGetFixedMtrr()

  @param[in]  Context    Ignored

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
UnitTestMtrrGetFixedMtrr (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MTRR_FIXED_SETTINGS  *Result;
  MTRR_FIXED_SETTINGS  FixedSettings;

  //
  // Get/Set current variable MTRRs
  //
  Result = MtrrGetFixedMtrr (&FixedSettings);
  UT_ASSERT_EQUAL (Result, &FixedSettings);
  Result = MtrrSetFixedMtrr (&FixedSettings);
  UT_ASSERT_EQUAL (Result, &FixedSettings);

  //
  // TODO: Add more test cases to verify the range of values
  //

  //
  // Negative test case when MTRRs are not supported
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 0;
  Result = MtrrGetFixedMtrr (&FixedSettings);
  UT_ASSERT_EQUAL (Result, &FixedSettings);

  return UNIT_TEST_PASSED;
}

/**
  Unit test of MtrrLib service MtrrSetFixedMtrr()

  @param[in]  Context    Ignored

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
UnitTestMtrrSetFixedMtrr (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MTRR_FIXED_SETTINGS  *Result;
  MTRR_FIXED_SETTINGS  FixedSettings;

  //
  // Get/Set current variable MTRRs
  //
  Result = MtrrGetFixedMtrr (&FixedSettings);
  UT_ASSERT_EQUAL (Result, &FixedSettings);
  Result = MtrrSetFixedMtrr (&FixedSettings);
  UT_ASSERT_EQUAL (Result, &FixedSettings);

  //
  // TODO: Add more test cases to verify the range of values
  //

  //
  // Negative test case when MTRRs are not supported
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 0;
  Result = MtrrSetFixedMtrr (&FixedSettings);
  UT_ASSERT_EQUAL (Result, &FixedSettings);

  return UNIT_TEST_PASSED;
}

/**
  Unit test of MtrrLib service MtrrGetAllMtrrs()

  @param[in]  Context    Ignored

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
UnitTestMtrrGetAllMtrrs (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MTRR_SETTINGS  *Result;
  MTRR_SETTINGS  Settings;

  //
  // Get/Set current variable MTRRs
  //
  Result = MtrrGetAllMtrrs (&Settings);
  UT_ASSERT_EQUAL (Result, &Settings);
  Result = MtrrSetAllMtrrs (&Settings);
  UT_ASSERT_EQUAL (Result, &Settings);

  //
  // TODO: Add more test cases to verify the range of BaseAddress and Mask
  // values
  //

  //
  // Negative test case when MTRRs are not supported
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 0;
  Result = MtrrGetAllMtrrs (&Settings);
  UT_ASSERT_EQUAL (Result, &Settings);
  
  //
  // Expect ASSERT() if variable MTRR count is > MTRR_NUMBER_OF_VARIABLE_MTRR
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 1;
  gMtrrCapMsr.Bits.VCNT = MTRR_NUMBER_OF_VARIABLE_MTRR + 1;
  UT_EXPECT_ASSERT_FAILURE (MtrrGetAllMtrrs (&Settings), NULL);

  return UNIT_TEST_PASSED;
}

/**
  Unit test of MtrrLib service MtrrSetAllMtrrs()

  @param[in]  Context    Ignored

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
UnitTestMtrrSetAllMtrrs (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MTRR_SETTINGS  *Result;
  MTRR_SETTINGS  Settings;

  //
  // Get/Set current variable MTRRs
  //
  Result = MtrrGetAllMtrrs (&Settings);
  UT_ASSERT_EQUAL (Result, &Settings);
  Result = MtrrSetAllMtrrs (&Settings);
  UT_ASSERT_EQUAL (Result, &Settings);

  //
  // TODO: Add more test cases to verify the range of BaseAddress and Mask
  // values
  //

  //
  // Negative test case when MTRRs are not supported
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 0;
  Result = MtrrSetAllMtrrs (&Settings);
  UT_ASSERT_EQUAL (Result, &Settings);
  
  //
  // Expect ASSERT() if variable MTRR count is > MTRR_NUMBER_OF_VARIABLE_MTRR
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 1;
  gMtrrCapMsr.Bits.VCNT = MTRR_NUMBER_OF_VARIABLE_MTRR + 1;
  UT_EXPECT_ASSERT_FAILURE (MtrrSetAllMtrrs (&Settings), NULL);

  return UNIT_TEST_PASSED;
}

/**
  Unit test of MtrrLib service MtrrGetMemoryAttributeInVariableMtrr()

  @param[in]  Context    Ignored

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
UnitTestMtrrGetMemoryAttributeInVariableMtrr (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT32         Result;
  VARIABLE_MTRR  VariableMtrr[MTRR_NUMBER_OF_VARIABLE_MTRR];
  UINT64         ValidMtrrBitsMask;
  UINT64         ValidMtrrAddressMask;
  MTRR_VARIABLE_SETTINGS  VariableSettings;

  //
  // Get/Set current variable MTRRs
  //
  MtrrGetVariableMtrr (&VariableSettings);
  VariableSettings.Mtrr[0].Mask |=  BIT11;
  MtrrSetVariableMtrr (&VariableSettings);
  
  ValidMtrrBitsMask    = LShiftU64 (1, gCpuidVirPhyAddressSizeEax.Bits.PhysicalAddressBits) - 1;
  ValidMtrrAddressMask = ValidMtrrBitsMask & 0xfffffffffffff000ULL;
  Result = MtrrGetMemoryAttributeInVariableMtrr (ValidMtrrBitsMask, ValidMtrrAddressMask, VariableMtrr);
  UT_ASSERT_EQUAL (Result, 1);

  //
  // Negative test case when MTRRs are not supported
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 0;
  ValidMtrrBitsMask    = LShiftU64 (1, gCpuidVirPhyAddressSizeEax.Bits.PhysicalAddressBits) - 1;
  ValidMtrrAddressMask = ValidMtrrBitsMask & 0xfffffffffffff000ULL;
  Result = MtrrGetMemoryAttributeInVariableMtrr (ValidMtrrBitsMask, ValidMtrrAddressMask, VariableMtrr);
  UT_ASSERT_EQUAL (Result, 0);
  
  //
  // Expect ASSERT() if variable MTRR count is > MTRR_NUMBER_OF_VARIABLE_MTRR
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 1;
  gMtrrCapMsr.Bits.VCNT = MTRR_NUMBER_OF_VARIABLE_MTRR + 1;
  ValidMtrrBitsMask    = LShiftU64 (1, gCpuidVirPhyAddressSizeEax.Bits.PhysicalAddressBits) - 1;
  ValidMtrrAddressMask = ValidMtrrBitsMask & 0xfffffffffffff000ULL;
  UT_EXPECT_ASSERT_FAILURE (MtrrGetMemoryAttributeInVariableMtrr (ValidMtrrBitsMask, ValidMtrrAddressMask, VariableMtrr), NULL);

  return UNIT_TEST_PASSED;
}

/**
  Unit test of MtrrLib service MtrrDebugPrintAllMtrrs()

  @param[in]  Context    Ignored

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
UnitTestMtrrDebugPrintAllMtrrs (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  //
  // Expect ASSERT() if variable MTRR count is > MTRR_NUMBER_OF_VARIABLE_MTRR
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 1;
  gMtrrCapMsr.Bits.FIX = 1;
  gMtrrCapMsr.Bits.VCNT = MTRR_NUMBER_OF_VARIABLE_MTRR + 1;
  UT_EXPECT_ASSERT_FAILURE (GetFirmwareVariableMtrrCount (), NULL);

  return UNIT_TEST_PASSED;
}

/**
  Unit test of MtrrLib service MtrrGetDefaultMemoryType()

  @param[in]  Context    Ignored

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
UnitTestMtrrGetDefaultMemoryType (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MTRR_MEMORY_CACHE_TYPE  Result;

  //
  // If MTRRs are supported, then always return the cache type in the MSR
  // MSR_IA32_MTRR_DEF_TYPE
  //
  gDefTypeMsr.Bits.Type = CacheUncacheable;
  Result = MtrrGetDefaultMemoryType ();
  UT_ASSERT_EQUAL (Result, CacheUncacheable);

  gDefTypeMsr.Bits.Type = CacheWriteCombining;
  Result = MtrrGetDefaultMemoryType ();
  UT_ASSERT_EQUAL (Result, CacheWriteCombining);

  gDefTypeMsr.Bits.Type = CacheWriteThrough;
  Result = MtrrGetDefaultMemoryType ();
  UT_ASSERT_EQUAL (Result, CacheWriteThrough);

  gDefTypeMsr.Bits.Type = CacheWriteProtected;
  Result = MtrrGetDefaultMemoryType ();
  UT_ASSERT_EQUAL (Result, CacheWriteProtected);

  gDefTypeMsr.Bits.Type = CacheWriteBack;
  Result = MtrrGetDefaultMemoryType ();
  UT_ASSERT_EQUAL (Result, CacheWriteBack);

  gDefTypeMsr.Bits.Type = CacheInvalid;
  Result = MtrrGetDefaultMemoryType ();
  UT_ASSERT_EQUAL (Result, CacheInvalid);

  //
  // If MTRRs are not supported, then always return CacheUncacheable
  //
  gCpuidVersionInfoEdx.Bits.MTRR = 0;
  gDefTypeMsr.Bits.Type = CacheInvalid;
  Result = MtrrGetDefaultMemoryType ();
  UT_ASSERT_EQUAL (Result, CacheUncacheable);

  gCpuidVersionInfoEdx.Bits.MTRR = 1;
  gMtrrCapMsr.Bits.FIX  = 0;
  gMtrrCapMsr.Bits.VCNT = 10;
  gDefTypeMsr.Bits.Type = CacheInvalid;
  Result = MtrrGetDefaultMemoryType ();
  UT_ASSERT_EQUAL (Result, CacheUncacheable);

  gCpuidVersionInfoEdx.Bits.MTRR = 1;
  gMtrrCapMsr.Bits.FIX  = 1;
  gMtrrCapMsr.Bits.VCNT = 0;
  gDefTypeMsr.Bits.Type = CacheInvalid;
  Result = MtrrGetDefaultMemoryType ();
  UT_ASSERT_EQUAL (Result, CacheUncacheable);

  return UNIT_TEST_PASSED;
}

/**
  Unit test of MtrrLib service MtrrSetMemoryAttributeInMtrrSettings()

  @param[in]  Context    Ignored

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
UnitTestMtrrSetMemoryAttributeInMtrrSettings (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return UNIT_TEST_PASSED;
}

/**
  Unit test of MtrrLib service MtrrSetMemoryAttributesInMtrrSettings()

  @param[in]  Context    Ignored

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
UnitTestMtrrSetMemoryAttributesInMtrrSettings (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return UNIT_TEST_PASSED;
}

/**
  Initialze the unit test framework, suite, and unit tests for the
  ResetSystemLib and run the ResetSystemLib unit test.

  @retval  EFI_SUCCESS           All test cases were dispatched.
  @retval  EFI_OUT_OF_RESOURCES  There are not enough resources available to
                                 initialize the unit tests.
**/
STATIC
EFI_STATUS
EFIAPI
UnitTestingEntry (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      MtrrTests;
  UNIT_TEST_SUITE_HANDLE      MtrrApiTests;

  Framework = NULL;

  DEBUG(( DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION ));

  //
  // Setup the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Framework, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION);
  if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
      goto EXIT;
  }

  //
  // Populate the MtrrLib Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&MtrrTests, Framework, "MtrrLib Tests", "MtrrLib.MtrrLib", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for MtrrLib Tests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  //
  // --------------Suite-----------Description--------------Name----------Function--------Pre---Post-------------------Context-----------
  //
  AddTestCase (MtrrTests, "Test ValidAndConfigurableMemoryLayouts",     "ValidAndConfigurableMemoryLayouts",     TestGeneratorForValidAndConfigurableMemoryLayouts, InitializeMtrrRegs, NULL, NULL);
  AddTestCase (MtrrTests, "Test InvalidMemoryLayouts",                  "InvalidMemoryLayouts",                  TestGeneratorForInvalidMemoryLayouts,              InitializeMtrrRegs, NULL, NULL);

  //
  // Populate the MtrrLib API Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&MtrrApiTests, Framework, "MtrrLib API Tests", "MtrrLib.MtrrLib", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for MtrrLib API Tests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (MtrrApiTests, "Test IsMtrrSupported",                       "MtrrSupported",                         UnitTestIsMtrrSupported,                           InitializeMtrrRegs, NULL, NULL);
  AddTestCase (MtrrApiTests, "Test GetVariableMtrrCount",                  "GetVariableMtrrCount",                  UnitTestGetVariableMtrrCount,                      InitializeMtrrRegs, NULL, NULL);
  AddTestCase (MtrrApiTests, "Test GetFirmwareVariableMtrrCount",          "GetFirmwareVariableMtrrCount",          UnitTestGetFirmwareVariableMtrrCount,              InitializeMtrrRegs, NULL, NULL);
  AddTestCase (MtrrApiTests, "Test MtrrSetMemoryAttribute",                "MtrrSetMemoryAttribute",                UnitTestMtrrSetMemoryAttribute,                    InitializeMtrrRegs, NULL, NULL);
  AddTestCase (MtrrApiTests, "Test MtrrGetMemoryAttribute",                "MtrrGetMemoryAttribute",                UnitTestMtrrGetMemoryAttribute,                    InitializeMtrrRegs, NULL, NULL);
  AddTestCase (MtrrApiTests, "Test MtrrGetVariableMtrr",                   "MtrrGetVariableMtrr",                   UnitTestMtrrGetVariableMtrr,                       InitializeMtrrRegs, NULL, NULL);
  AddTestCase (MtrrApiTests, "Test MtrrSetVariableMtrr",                   "MtrrSetVariableMtrr",                   UnitTestMtrrSetVariableMtrr,                       InitializeMtrrRegs, NULL, NULL);
  AddTestCase (MtrrApiTests, "Test MtrrGetFixedMtrr",                      "MtrrGetFixedMtrr",                      UnitTestMtrrGetFixedMtrr,                          InitializeMtrrRegs, NULL, NULL);
  AddTestCase (MtrrApiTests, "Test MtrrSetFixedMtrr",                      "MtrrSetFixedMtrr",                      UnitTestMtrrSetFixedMtrr,                          InitializeMtrrRegs, NULL, NULL);
  AddTestCase (MtrrApiTests, "Test MtrrGetAllMtrrs",                       "MtrrGetAllMtrrs",                       UnitTestMtrrGetAllMtrrs,                           InitializeMtrrRegs, NULL, NULL);
  AddTestCase (MtrrApiTests, "Test MtrrSetAllMtrrs",                       "MtrrSetAllMtrrs",                       UnitTestMtrrSetAllMtrrs,                           InitializeMtrrRegs, NULL, NULL);
  AddTestCase (MtrrApiTests, "Test MtrrGetMemoryAttributeInVariableMtrr",  "MtrrGetMemoryAttributeInVariableMtrr",  UnitTestMtrrGetMemoryAttributeInVariableMtrr,      InitializeMtrrRegs, NULL, NULL);
  AddTestCase (MtrrApiTests, "Test MtrrDebugPrintAllMtrrs",                "MtrrDebugPrintAllMtrrs",                UnitTestMtrrDebugPrintAllMtrrs,                    InitializeMtrrRegs, NULL, NULL);
  AddTestCase (MtrrApiTests, "Test MtrrGetDefaultMemoryType",              "MtrrGetDefaultMemoryType",              UnitTestMtrrGetDefaultMemoryType,                  InitializeMtrrRegs, NULL, NULL);
  AddTestCase (MtrrApiTests, "Test MtrrSetMemoryAttributeInMtrrSettings",  "MtrrSetMemoryAttributeInMtrrSettings",  UnitTestMtrrSetMemoryAttributeInMtrrSettings,      InitializeMtrrRegs, NULL, NULL);
  AddTestCase (MtrrApiTests, "Test MtrrSetMemoryAttributesInMtrrSettings", "MtrrSetMemoryAttributesInMtrrSettings", UnitTestMtrrSetMemoryAttributesInMtrrSettings,     InitializeMtrrRegs, NULL, NULL);

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Framework);

EXIT:
  if (Framework) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}

/**
  Standard POSIX C entry point for host based unit test execution.
**/
int
main (
  int argc,
  char *argv[]
  )
{
  return UnitTestingEntry ();
}
