/** @file
  GUID and structure for DXE Core Timer Tick diagnostics.

  Installed in the EFI System Configuration Table to allow external test
  applications to observe CoreTimerTick() nesting behavior without requiring
  a custom protocol.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#define DXE_CORE_TIMER_TICK_DIAGNOSTICS_GUID \
  { 0x7C3E8A2D, 0x5F19, 0x4B6E, { 0xA1, 0xD4, 0x82, 0xF6, 0x3C, 0x09, 0xE7, 0xB5 } }

extern EFI_GUID  gDxeCoreTimerTickDiagnosticsGuid;

///
/// Signature for CORE_TIMER_TICK_DIAGNOSTICS structure validation.
///
#define CORE_TIMER_TICK_DIAGNOSTICS_SIGNATURE  SIGNATURE_32('T','T','D','G')

///
/// Structure exposed via EFI System Configuration Table that tracks
/// CoreTimerTick() recursion depth and per-TPL-level entry counts.
///
/// All fields are volatile to ensure visibility across interrupt contexts.
///
typedef struct {
  UINT32            Signature;        ///< Must be CORE_TIMER_TICK_DIAGNOSTICS_SIGNATURE
  volatile UINTN    CurrentDepth;     ///< Current active CoreTimerTick() frame depth (0 when idle)
  volatile UINTN    MaxDepth;         ///< Maximum CoreTimerTick() frame depth observed since boot
  volatile UINTN    TotalEntries;     ///< Total number of CoreTimerTick() entries
  ///
  /// Per-TPL entry counters.  Index is the TPL value at which CoreTimerTick
  /// was entered (the interrupted TPL when Timer Arch Protocol did not raise
  /// TPL, or the TPL recorded in gTplBeforeHighTpl when it did).
  /// Only indices 0..31 are valid.
  ///
  volatile UINTN    EntriesAtTpl[32];
} CORE_TIMER_TICK_DIAGNOSTICS;
