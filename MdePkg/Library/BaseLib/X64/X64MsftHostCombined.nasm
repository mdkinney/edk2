//------------------------------------------------------------------------------
// Combined X64 NASM MSFT Host-Safe Functions
//------------------------------------------------------------------------------
#include "BaseLibInternals.h"

    DEFAULT REL
    SECTION .text

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; CpuPause (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(CpuPause)
ASM_PFX(CpuPause):
    pause
    ret

;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmReadTsc (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadTsc)
ASM_PFX(AsmReadTsc):
    rdtsc
    shl     rdx, 0x20
    or      rax, rdx
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

global ASM_PFX(AsmWriteMm6)
ASM_PFX(AsmWriteMm6):
    movq    mm6, rcx
    ret

global ASM_PFX(AsmWriteMm5)
ASM_PFX(AsmWriteMm5):
    movq    mm5, rcx
    ret

global ASM_PFX(AsmWriteMm4)
ASM_PFX(AsmWriteMm4):
    movq    mm4, rcx
    ret

global ASM_PFX(AsmWriteMm3)
ASM_PFX(AsmWriteMm3):
    movq    mm3, rcx
    ret

global ASM_PFX(AsmWriteMm2)
ASM_PFX(AsmWriteMm2):
    movq    mm2, rcx
    ret

global ASM_PFX(AsmWriteMm1)
ASM_PFX(AsmWriteMm1):
    movq    mm1, rcx
    ret

global ASM_PFX(AsmWriteMm0)
ASM_PFX(AsmWriteMm0):
    movq    mm0, rcx
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

global ASM_PFX(AsmReadMm6)
ASM_PFX(AsmReadMm6):
    movq    rax, mm6
    ret

global ASM_PFX(AsmReadMm5)
ASM_PFX(AsmReadMm5):
    movq    rax, mm5
    ret

global ASM_PFX(AsmReadMm4)
ASM_PFX(AsmReadMm4):
    movq    rax, mm4
    ret

global ASM_PFX(AsmReadMm3)
ASM_PFX(AsmReadMm3):
    movq    rax, mm3
    ret

global ASM_PFX(AsmReadMm2)
ASM_PFX(AsmReadMm2):
    movq    rax, mm2
    ret

global ASM_PFX(AsmReadMm1)
ASM_PFX(AsmReadMm1):
    movq    rax, mm1
    ret

global ASM_PFX(AsmReadMm0)
ASM_PFX(AsmReadMm0):
    movq    rax, mm0
    ret

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; InternalX86FxRestore (
;   IN      CONST IA32_FX_BUFFER    *Buffer
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalX86FxRestore)
ASM_PFX(InternalX86FxRestore):
    fxrstor [rcx]
    ret

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; InternalX86FxSave (
;   OUT     IA32_FX_BUFFER    *Buffer
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalX86FxSave)
ASM_PFX(InternalX86FxSave):
    fxsave  [rcx]
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
