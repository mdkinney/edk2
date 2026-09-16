//------------------------------------------------------------------------------
// Combined Ia32 NASM Functions (GCC Toolchains)
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
; UINT64
; EFIAPI
; InternalMathSwapBytes64 (
;   IN      UINT64                    Operand
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMathSwapBytes64)
ASM_PFX(InternalMathSwapBytes64):
    mov     eax, [esp + 8]              ; eax <- upper 32 bits
    mov     edx, [esp + 4]              ; edx <- lower 32 bits
    bswap   eax
    bswap   edx
    ret


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; InternalMathDivRemU64x32 (
;   IN      UINT64                    Dividend,
;   IN      UINT32                    Divisor,
;   OUT     UINT32                    *Remainder
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMathDivRemU64x32)
ASM_PFX(InternalMathDivRemU64x32):
    mov     ecx, [esp + 12]         ; ecx <- divisor
    mov     eax, [esp + 8]          ; eax <- dividend[32..63]
    xor     edx, edx
    div     ecx                     ; eax <- quotient[32..63], edx <- remainder
    push    eax
    mov     eax, [esp + 8]          ; eax <- dividend[0..31]
    div     ecx                     ; eax <- quotient[0..31]
    mov     ecx, [esp + 20]         ; ecx <- Remainder
    jecxz   .0                      ; abandon remainder if Remainder == NULL
    mov     [ecx], edx
.0:
    pop     edx                     ; edx <- quotient[32..63]
    ret


;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; InternalMathModU64x32 (
;   IN      UINT64                    Dividend,
;   IN      UINT32                    Divisor
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMathModU64x32)
ASM_PFX(InternalMathModU64x32):
    mov     eax, [esp + 8]
    mov     ecx, [esp + 12]
    xor     edx, edx
    div     ecx
    mov     eax, [esp + 4]
    div     ecx
    mov     eax, edx
    ret


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; InternalMathDivU64x32 (
;   IN      UINT64                    Dividend,
;   IN      UINT32                    Divisor
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMathDivU64x32)
ASM_PFX(InternalMathDivU64x32):
    mov     eax, [esp + 8]
    mov     ecx, [esp + 12]
    xor     edx, edx
    div     ecx
    push    eax                     ; save quotient on stack
    mov     eax, [esp + 8]
    div     ecx
    pop     edx                     ; restore high-order dword of the quotient
    ret


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; InternalMathMultU64x64 (
;   IN      UINT64                    Multiplicand,
;   IN      UINT64                    Multiplier
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMathMultU64x64)
ASM_PFX(InternalMathMultU64x64):
    push    ebx
    mov     ebx, [esp + 8]              ; ebx <- M1[0..31]
    mov     edx, [esp + 16]             ; edx <- M2[0..31]
    mov     ecx, ebx
    mov     eax, edx
    imul    ebx, [esp + 20]             ; ebx <- M1[0..31] * M2[32..63]
    imul    edx, [esp + 12]             ; edx <- M1[32..63] * M2[0..31]
    add     ebx, edx                    ; carries are abandoned
    mul     ecx                         ; edx:eax <- M1[0..31] * M2[0..31]
    add     edx, ebx                    ; carries are abandoned
    pop     ebx
    ret


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; InternalMathMultU64x32 (
;   IN      UINT64                    Multiplicand,
;   IN      UINT32                    Multiplier
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMathMultU64x32)
ASM_PFX(InternalMathMultU64x32):
    mov     ecx, [esp + 12]
    mov     eax, ecx
    imul    ecx, [esp + 8]              ; overflow not detectable
    mul     dword [esp + 4]
    add     edx, ecx
    ret


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; InternalMathRRotU64 (
;   IN      UINT64                    Operand,
;   IN      UINTN                     Count
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMathRRotU64)
ASM_PFX(InternalMathRRotU64):
    push    ebx
    mov     cl, [esp + 16]
    mov     eax, [esp + 8]
    mov     edx, [esp + 12]
    shrd    ebx, eax, cl
    shrd    eax, edx, cl
    rol     ebx, cl
    shrd    edx, ebx, cl
    test    cl, 32                      ; Count >= 32?
    jz      .0
    mov     ecx, eax                    ; switch eax & edx if Count >= 32
    mov     eax, edx
    mov     edx, ecx
.0:
    pop     ebx
    ret


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; InternalMathLRotU64 (
;   IN      UINT64                    Operand,
;   IN      UINTN                     Count
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMathLRotU64)
ASM_PFX(InternalMathLRotU64):
    push    ebx
    mov     cl, [esp + 16]
    mov     edx, [esp + 12]
    mov     eax, [esp + 8]
    shld    ebx, edx, cl
    shld    edx, eax, cl
    ror     ebx, cl
    shld    eax, ebx, cl
    test    cl, 32                      ; Count >= 32?
    jz      .0
    mov     ecx, eax
    mov     eax, edx
    mov     edx, ecx
.0:
    pop     ebx
    ret


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; InternalMathARShiftU64 (
;   IN      UINT64                    Operand,
;   IN      UINTN                     Count
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMathARShiftU64)
ASM_PFX(InternalMathARShiftU64):
    mov     cl, [esp + 12]
    mov     eax, [esp + 8]
    cdq
    test    cl, 32
    jnz     .0
    mov     edx, eax
    mov     eax, [esp + 4]
.0:
    shrd    eax, edx, cl
    sar     edx, cl
    ret


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; InternalMathRShiftU64 (
;   IN      UINT64                    Operand,
;   IN      UINTN                     Count
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMathRShiftU64)
ASM_PFX(InternalMathRShiftU64):
    mov     cl, [esp + 12]              ; cl <- Count
    xor     edx, edx
    mov     eax, [esp + 8]
    test    cl, 32                      ; Count >= 32?
    jnz     .0
    mov     edx, eax
    mov     eax, [esp + 4]
.0:
    shrd    eax, edx, cl
    shr     edx, cl
    ret


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; InternalMathLShiftU64 (
;   IN      UINT64                    Operand,
;   IN      UINTN                     Count
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMathLShiftU64)
ASM_PFX(InternalMathLShiftU64):
    mov     cl, [esp + 12]
    xor     eax, eax
    mov     edx, [esp + 4]
    test    cl, 32                      ; Count >= 32?
    jnz     .0
    mov     eax, edx
    mov     edx, [esp + 8]
.0:
    shld    edx, eax, cl
    shl     eax, cl
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


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; InternalSwitchStack (
;   IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
;   IN      VOID                      *Context1,   OPTIONAL
;   IN      VOID                      *Context2,   OPTIONAL
;   IN      VOID                      *NewStack
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalSwitchStack)
ASM_PFX(InternalSwitchStack):
  push  ebp
  mov   ebp, esp

  mov   esp, [ebp + 20]    ; switch stack
  sub   esp, 8
  mov   eax, [ebp + 16]
  mov   [esp + 4], eax
  mov   eax, [ebp + 12]
  mov   [esp], eax
  push  0                  ; keeps gdb from unwinding stack
  jmp   dword [ebp + 8]    ; call and never return
