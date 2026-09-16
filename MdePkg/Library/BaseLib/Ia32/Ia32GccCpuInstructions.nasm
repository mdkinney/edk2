//------------------------------------------------------------------------------
// Combined Ia32 NASM CPU Instructions (GCC)
//------------------------------------------------------------------------------
#include "BaseLibInternals.h"

    DEFAULT REL
    SECTION .text


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; EnableDisableInterrupts (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(EnableDisableInterrupts)
ASM_PFX(EnableDisableInterrupts):
    sti
    cli
    ret


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; InternalX86DisablePaging32 (
;   IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
;   IN      VOID                      *Context1,    OPTIONAL
;   IN      VOID                      *Context2,    OPTIONAL
;   IN      VOID                      *NewStack
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalX86DisablePaging32)
ASM_PFX(InternalX86DisablePaging32):
    mov     ebx, [esp + 4]
    mov     ecx, [esp + 8]
    mov     edx, [esp + 12]
    pushfd
    pop     edi                         ; save EFLAGS to edi
    cli
    mov     eax, cr0
    btr     eax, 31
    mov     esp, [esp + 16]
    mov     cr0, eax
    push    edi
    popfd                               ; restore EFLAGS from edi
    push    edx
    push    ecx
    call    ebx
    jmp     $                           ; EntryPoint() should not return


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; InternalX86EnablePaging32 (
;   IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
;   IN      VOID                      *Context1,    OPTIONAL
;   IN      VOID                      *Context2,    OPTIONAL
;   IN      VOID                      *NewStack
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalX86EnablePaging32)
ASM_PFX(InternalX86EnablePaging32):
    mov     ebx, [esp + 4]
    mov     ecx, [esp + 8]
    mov     edx, [esp + 12]
    pushfd
    pop     edi                         ; save flags in edi
    cli
    mov     eax, cr0
    bts     eax, 31
    mov     esp, [esp + 16]
    mov     cr0, eax
    push    edi
    popfd                               ; restore flags
    push    edx
    push    ecx
    call    ebx
    jmp     $


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmMwait (
;   IN      UINTN                     Eax,
;   IN      UINTN                     Ecx
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmMwait)
ASM_PFX(AsmMwait):
    mov     eax, [esp + 4]
    mov     ecx, [esp + 8]
    mwait
    ret


;------------------------------------------------------------------------------
; UINTN
; EFIAPI
; AsmMonitor (
;   IN      UINTN                     Eax,
;   IN      UINTN                     Ecx,
;   IN      UINTN                     Edx
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmMonitor)
ASM_PFX(AsmMonitor):
    mov     eax, [esp + 4]
    mov     ecx, [esp + 8]
    mov     edx, [esp + 12]
    monitor
    ret


;------------------------------------------------------------------------------
;  UINT32
;  EFIAPI
;  AsmCpuidEx (
;    IN   UINT32  RegisterInEax,
;    IN   UINT32  RegisterInEcx,
;    OUT  UINT32  *RegisterOutEax  OPTIONAL,
;    OUT  UINT32  *RegisterOutEbx  OPTIONAL,
;    OUT  UINT32  *RegisterOutEcx  OPTIONAL,
;    OUT  UINT32  *RegisterOutEdx  OPTIONAL
;    )
;------------------------------------------------------------------------------
global ASM_PFX(AsmCpuidEx)
ASM_PFX(AsmCpuidEx):
    push    ebx
    push    ebp
    mov     ebp, esp
    mov     eax, [ebp + 12]
    mov     ecx, [ebp + 16]
    cpuid
    push    ecx
    mov     ecx, [ebp + 20]
    jecxz   .0
    mov     [ecx], eax
.0:
    mov     ecx, [ebp + 24]
    jecxz   .1
    mov     [ecx], ebx
.1:
    mov     ecx, [ebp + 32]
    jecxz   .2
    mov     [ecx], edx
.2:
    mov     ecx, [ebp + 28]
    jecxz   .3
    pop     DWORD [ecx]
.3:
    mov     eax, [ebp + 12]
    leave
    pop     ebx
    ret


;------------------------------------------------------------------------------
;  VOID
;  EFIAPI
;  AsmCpuid (
;    IN   UINT32  RegisterInEax,
;    OUT  UINT32  *RegisterOutEax  OPTIONAL,
;    OUT  UINT32  *RegisterOutEbx  OPTIONAL,
;    OUT  UINT32  *RegisterOutEcx  OPTIONAL,
;    OUT  UINT32  *RegisterOutEdx  OPTIONAL
;    );
;------------------------------------------------------------------------------
global ASM_PFX(AsmCpuid)
ASM_PFX(AsmCpuid):
    push    ebx
    push    ebp
    mov     ebp, esp
    mov     eax, [ebp + 12]
    cpuid
    push    ecx
    mov     ecx, [ebp + 16]
    jecxz   .0
    mov     [ecx], eax
.0:
    mov     ecx, [ebp + 20]
    jecxz   .1
    mov     [ecx], ebx
.1:
    mov     ecx, [ebp + 24]
    jecxz   .2
    pop     DWORD [ecx]
.2:
    mov     ecx, [ebp + 28]
    jecxz   .3
    mov     [ecx], edx
.3:
    mov     eax, [ebp + 12]
    leave
    pop     ebx
    ret


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmEnableCache (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmEnableCache)
ASM_PFX(AsmEnableCache):
    wbinvd
    mov     eax, cr0
    btr     eax, 29
    btr     eax, 30
    mov     cr0, eax
    ret


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmDisableCache (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmDisableCache)
ASM_PFX(AsmDisableCache):
    mov     eax, cr0
    bts     eax, 30
    btr     eax, 29
    mov     cr0, eax
    wbinvd
    ret

