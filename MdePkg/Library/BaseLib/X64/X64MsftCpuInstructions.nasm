//------------------------------------------------------------------------------
// Combined X64 NASM CPU Instructions (MSFT specific)
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
; VOID
; EFIAPI
; DisableInterrupts (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(DisableInterrupts)
ASM_PFX(DisableInterrupts):
    cli
    ret


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; EnableInterrupts (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(EnableInterrupts)
ASM_PFX(EnableInterrupts):
    sti
    ret


;------------------------------------------------------------------------------
; VOID *
; EFIAPI
; AsmFlushCacheLine (
;   IN      VOID                      *LinearAddress
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmFlushCacheLine)
ASM_PFX(AsmFlushCacheLine):
    clflush [rcx]
    mov     rax, rcx
    ret


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmInvd (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmInvd)
ASM_PFX(AsmInvd):
    invd
    ret


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmWbinvd (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWbinvd)
ASM_PFX(AsmWbinvd):
    wbinvd
    ret


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmMwait (
;   IN      UINTN                     Eax,
;   IN      UINTN                     Ecx
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmMwait)
ASM_PFX(AsmMwait):
    mov     eax, ecx
    mov     ecx, edx
    mwait
    ret


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmMonitor (
;   IN      UINTN                     Eax,
;   IN      UINTN                     Ecx,
;   IN      UINTN                     Edx
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmMonitor)
ASM_PFX(AsmMonitor):
    mov     eax, ecx
    mov     ecx, edx
    mov     edx, r8d
    monitor
    ret


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmReadPmc (
;   IN UINT32   PmcIndex
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadPmc)
ASM_PFX(AsmReadPmc):
    rdpmc
    shl     rdx, 0x20
    or      rax, rdx
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
; InternalX86FxRestore (
;   IN CONST IA32_FX_BUFFER *Buffer
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
;   OUT IA32_FX_BUFFER *Buffer
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalX86FxSave)
ASM_PFX(InternalX86FxSave):
    fxsave  [rcx]
    ret


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; CpuBreakpoint (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(CpuBreakpoint)
ASM_PFX(CpuBreakpoint):
    int  3
    ret

