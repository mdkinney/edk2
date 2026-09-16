;------------------------------------------------------------------------------
;
; Copyright (c) 2006 - 2026, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   X64RegisterAccessors.nasm
;
; Abstract:
;
;   Combined X64 NASM assembly functions for Control Registers, Debug Registers,
;   MMX Registers, and Descriptor Table Registers / Segment Bases.
;
;------------------------------------------------------------------------------

    DEFAULT REL
    SECTION .text


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmReadCr0 (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadCr0)
ASM_PFX(AsmReadCr0):
    mov     rax, cr0
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmReadCr2 (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadCr2)
ASM_PFX(AsmReadCr2):
    mov     rax, cr2
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmReadCr3 (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadCr3)
ASM_PFX(AsmReadCr3):
    mov     rax, cr3
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmReadCr4 (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadCr4)
ASM_PFX(AsmReadCr4):
    mov     rax, cr4
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmWriteCr0 (
;   UINTN  Cr0
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteCr0)
ASM_PFX(AsmWriteCr0):
    mov     cr0, rcx
    mov     rax, rcx
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmWriteCr2 (
;   UINTN  Cr2
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteCr2)
ASM_PFX(AsmWriteCr2):
    mov     cr2, rcx
    mov     rax, rcx
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmWriteCr3 (
;   UINTN  Cr3
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteCr3)
ASM_PFX(AsmWriteCr3):
    mov     cr3, rcx
    mov     rax, rcx
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmWriteCr4 (
;   UINTN  Cr4
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteCr4)
ASM_PFX(AsmWriteCr4):
    mov     cr4, rcx
    mov     rax, rcx
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmReadEflags (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadEflags)
ASM_PFX(AsmReadEflags):
    pushfq
    pop     rax
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmReadDr0 (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadDr0)
ASM_PFX(AsmReadDr0):
    mov     rax, dr0
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmReadDr1 (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadDr1)
ASM_PFX(AsmReadDr1):
    mov     rax, dr1
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmReadDr2 (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadDr2)
ASM_PFX(AsmReadDr2):
    mov     rax, dr2
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmReadDr3 (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadDr3)
ASM_PFX(AsmReadDr3):
    mov     rax, dr3
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmReadDr4 (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadDr4)
ASM_PFX(AsmReadDr4):
    ;
    ; There's no obvious reason to access this register, since it's aliased to
    ; DR7 when DE=0 or an exception generated when DE=1
    ;
    mov     rax, dr4
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmReadDr5 (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadDr5)
ASM_PFX(AsmReadDr5):
    ;
    ; There's no obvious reason to access this register, since it's aliased to
    ; DR7 when DE=0 or an exception generated when DE=1
    ;
    mov     rax, dr5
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmReadDr6 (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadDr6)
ASM_PFX(AsmReadDr6):
    mov     rax, dr6
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmReadDr7 (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadDr7)
ASM_PFX(AsmReadDr7):
    mov     rax, dr7
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmWriteDr0 (
;   IN UINTN Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteDr0)
ASM_PFX(AsmWriteDr0):
    mov     dr0, rcx
    mov     rax, rcx
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmWriteDr1 (
;   IN UINTN Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteDr1)
ASM_PFX(AsmWriteDr1):
    mov     dr1, rcx
    mov     rax, rcx
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmWriteDr2 (
;   IN UINTN Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteDr2)
ASM_PFX(AsmWriteDr2):
    mov     dr2, rcx
    mov     rax, rcx
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmWriteDr3 (
;   IN UINTN Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteDr3)
ASM_PFX(AsmWriteDr3):
    mov     dr3, rcx
    mov     rax, rcx
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmWriteDr4 (
;   IN UINTN Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteDr4)
ASM_PFX(AsmWriteDr4):
    ;
    ; There's no obvious reason to access this register, since it's aliased to
    ; DR6 when DE=0 or an exception generated when DE=1
    ;
    mov     dr4, rcx
    mov     rax, rcx
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmWriteDr5 (
;   IN UINTN Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteDr5)
ASM_PFX(AsmWriteDr5):
    ;
    ; There's no obvious reason to access this register, since it's aliased to
    ; DR7 when DE=0 or an exception generated when DE=1
    ;
    mov     dr5, rcx
    mov     rax, rcx
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmWriteDr6 (
;   IN UINTN Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteDr6)
ASM_PFX(AsmWriteDr6):
    mov     dr6, rcx
    mov     rax, rcx
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmWriteDr7 (
;   IN UINTN Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteDr7)
ASM_PFX(AsmWriteDr7):
    mov     dr7, rcx
    mov     rax, rcx
    ret


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmReadMm0 (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadMm0)
ASM_PFX(AsmReadMm0):
    movq    rax, mm0
    ret


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmReadMm1 (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadMm1)
ASM_PFX(AsmReadMm1):
    movq    rax, mm1
    ret


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmReadMm2 (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadMm2)
ASM_PFX(AsmReadMm2):
    movq    rax, mm2
    ret


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmReadMm3 (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadMm3)
ASM_PFX(AsmReadMm3):
    movq    rax, mm3
    ret


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmReadMm4 (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadMm4)
ASM_PFX(AsmReadMm4):
    movq    rax, mm4
    ret


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmReadMm5 (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadMm5)
ASM_PFX(AsmReadMm5):
    movq    rax, mm5
    ret


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmReadMm6 (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadMm6)
ASM_PFX(AsmReadMm6):
    movq    rax, mm6
    ret


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmReadMm7 (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadMm7)
ASM_PFX(AsmReadMm7):
    movq    rax, mm7
    ret


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmWriteMm0 (
;   IN UINT64   Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteMm0)
ASM_PFX(AsmWriteMm0):
    movq    mm0, rcx
    ret


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmWriteMm1 (
;   IN UINT64   Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteMm1)
ASM_PFX(AsmWriteMm1):
    movq    mm1, rcx
    ret


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmWriteMm2 (
;   IN UINT64   Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteMm2)
ASM_PFX(AsmWriteMm2):
    movq    mm2, rcx
    ret


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmWriteMm3 (
;   IN UINT64   Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteMm3)
ASM_PFX(AsmWriteMm3):
    movq    mm3, rcx
    ret


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmWriteMm4 (
;   IN UINT64   Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteMm4)
ASM_PFX(AsmWriteMm4):
    movq    mm4, rcx
    ret


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmWriteMm5 (
;   IN UINT64   Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteMm5)
ASM_PFX(AsmWriteMm5):
    movq    mm5, rcx
    ret


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmWriteMm6 (
;   IN UINT64   Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteMm6)
ASM_PFX(AsmWriteMm6):
    movq    mm6, rcx
    ret


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmWriteMm7 (
;   IN UINT64   Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteMm7)
ASM_PFX(AsmWriteMm7):
    movq    mm7, rcx
    ret


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; InternalX86ReadGdtr (
;   OUT IA32_DESCRIPTOR  *Gdtr
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalX86ReadGdtr)
ASM_PFX(InternalX86ReadGdtr):
    sgdt    [rcx]
    ret


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; InternalX86WriteGdtr (
;   IN      CONST IA32_DESCRIPTOR     *Idtr
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalX86WriteGdtr)
ASM_PFX(InternalX86WriteGdtr):
    lgdt    [rcx]
    ret


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; InternalX86ReadIdtr (
;   OUT     IA32_DESCRIPTOR           *Idtr
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalX86ReadIdtr)
ASM_PFX(InternalX86ReadIdtr):
    sidt    [rcx]
    ret


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; InternalX86WriteIdtr (
;   IN      CONST IA32_DESCRIPTOR     *Idtr
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalX86WriteIdtr)
ASM_PFX(InternalX86WriteIdtr):
    pushfq
    cli
    lidt    [rcx]
    popfq
    ret


;------------------------------------------------------------------------------
; UINT16
; EFIAPI
; AsmReadLdtr (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadLdtr)
ASM_PFX(AsmReadLdtr):
    sldt    eax
    ret


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmWriteLdtr (
;   IN UINT16 Ldtr
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteLdtr)
ASM_PFX(AsmWriteLdtr):
    lldt    cx
    ret


;------------------------------------------------------------------------------
; UINT16
; EFIAPI
; AsmReadTr (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadTr)
ASM_PFX(AsmReadTr):
    str     eax
    ret


;------------------------------------------------------------------------------
; VOID
; AsmWriteTr (
;   UINT16 Selector
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteTr)
ASM_PFX(AsmWriteTr):
    mov     eax, ecx
    ltr     ax
    ret


;------------------------------------------------------------------------------
; UINT16
; EFIAPI
; AsmReadCs (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadCs)
ASM_PFX(AsmReadCs):
    mov     eax, cs
    ret


;------------------------------------------------------------------------------
; UINT16
; EFIAPI
; AsmReadDs (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadDs)
ASM_PFX(AsmReadDs):
    mov     eax, ds
    ret


;------------------------------------------------------------------------------
; UINT16
; EFIAPI
; AsmReadEs (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadEs)
ASM_PFX(AsmReadEs):
    mov     eax, es
    ret


;------------------------------------------------------------------------------
; UINT16
; EFIAPI
; AsmReadFs (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadFs)
ASM_PFX(AsmReadFs):
    mov     eax, fs
    ret


;------------------------------------------------------------------------------
; UINT16
; EFIAPI
; AsmReadGs (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadGs)
ASM_PFX(AsmReadGs):
    mov     eax, gs
    ret


;------------------------------------------------------------------------------
; UINT16
; EFIAPI
; AsmReadSs (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadSs)
ASM_PFX(AsmReadSs):
    mov     eax, ss
    ret




;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmReadMsr64 (
;   IN UINT32  Index
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadMsr64)
ASM_PFX(AsmReadMsr64):
    rdmsr                               ; edx & eax are zero extended
    shl     rdx, 0x20
    or      rax, rdx
    ret


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmWriteMsr64 (
;   IN UINT32  Index,
;   IN UINT64  Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteMsr64)
ASM_PFX(AsmWriteMsr64):
    mov     rax, rdx                    ; meanwhile, rax <- return value
    shr     rdx, 0x20                    ; edx:eax contains the value to write
    wrmsr
    ret

