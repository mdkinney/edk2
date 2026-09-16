//------------------------------------------------------------------------------
// Combined X64 NASM CPU Misc Instructions
//------------------------------------------------------------------------------
#include "BaseLibInternals.h"

    DEFAULT REL
    SECTION .text


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
    mov     rax, cr0
    btr     rax, 29
    btr     rax, 30
    mov     cr0, rax
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
    mov     rax, cr0
    bts     rax, 30
    btr     rax, 29
    mov     cr0, rax
    wbinvd
    ret


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmLfence (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmLfence)
ASM_PFX(AsmLfence):
    lfence
    ret

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
; InternalX86DisablePaging64 (
;   IN      UINT16                    Cs,
;   IN      UINT32                    EntryPoint,
;   IN      UINT32                    Context1,  OPTIONAL
;   IN      UINT32                    Context2,  OPTIONAL
;   IN      UINT32                    NewStack
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalX86DisablePaging64)
ASM_PFX(InternalX86DisablePaging64):
    cli
    lea     rsi, [.0]                     ; rsi <- The start address of transition code
    mov     edi, [rsp + 0x28]            ; rdi <- New stack
    lea     rax, [mTransitionEnd]         ; rax <- end of transition code
    sub     rax, rsi                    ; rax <- The size of transition piece code
    add     rax, 4                      ; Round RAX up to the next 4 byte boundary
    and     al, 0xfc
    sub     rdi, rax                    ; rdi <- Use stack to hold transition code
    mov     r10d, edi                   ; r10 <- The start address of transition code below 4G
    push    rcx                         ; save rcx to stack
    mov     rcx, rax                    ; rcx <- The size of transition piece code
    rep     movsb                       ; copy transition code to top of new stack which must be below 4GB
    pop     rcx                         ; restore rcx

    mov     esi, r8d
    mov     edi, r9d
    mov     eax, r10d                   ; eax <- start of the transition code on the stack
    sub     eax, 4                      ; eax <- One slot below transition code on the stack
    push    rcx                         ; push Cs to stack
    push    r10                         ; push address of tansition code on stack
    retfq

; Start of transition code
.0:
    mov     esp, eax                    ; set up new stack
    mov     rax, cr0
    btr     eax, 31                     ; Clear CR0.PG
    mov     cr0, rax                    ; disable paging and caches

    mov     ebx, edx                    ; save EntryPoint to rbx, for rdmsr will overwrite rdx
    mov     ecx, 0xc0000080
    rdmsr
    and     ah, ~ 1                   ; clear LME
    wrmsr
    mov     rax, cr4
    and     al, ~ (1 << 5)           ; clear PAE
    mov     cr4, rax
    push    rdi                         ; push Context2
    push    rsi                         ; push Context1
    call    rbx                         ; transfer control to EntryPoint
    hlt                                 ; no one should get here

mTransitionEnd:


;------------------------------------------------------------------------------
;  Generates a 16 bit random number through RDRAND instruction.
;  Return TRUE if Rand generated successfully, or FALSE if not.
;
;  BOOLEAN EFIAPI InternalX86RdRand16 (UINT16 *Rand);
;------------------------------------------------------------------------------
global ASM_PFX(InternalX86RdRand16)
ASM_PFX(InternalX86RdRand16):
    rdrand eax                     ; generate a 16 bit RN into eax,
                                   ; CF=1 if RN generated ok, otherwise CF=0
    jc     rn16_ok                 ; jmp if CF=1
    xor    rax, rax                ; reg=0 if CF=0
    ret                            ; return with failure status
rn16_ok:
    mov    [rcx], ax
    mov    rax,  1
    ret

;------------------------------------------------------------------------------
;  Generates a 32 bit random number through RDRAND instruction.
;  Return TRUE if Rand generated successfully, or FALSE if not.
;
;  BOOLEAN EFIAPI InternalX86RdRand32 (UINT32 *Rand);
;------------------------------------------------------------------------------
global ASM_PFX(InternalX86RdRand32)
ASM_PFX(InternalX86RdRand32):
    rdrand eax                     ; generate a 32 bit RN into eax,
                                   ; CF=1 if RN generated ok, otherwise CF=0
    jc     rn32_ok                 ; jmp if CF=1
    xor    rax, rax                ; reg=0 if CF=0
    ret                            ; return with failure status
rn32_ok:
    mov    [rcx], eax
    mov    rax,  1
    ret

;------------------------------------------------------------------------------
;  Generates a 64 bit random number through one RDRAND instruction.
;  Return TRUE if Rand generated successfully, or FALSE if not.
;
;  BOOLEAN EFIAPI InternalX86RdRand64 (UINT64 *Random);
;------------------------------------------------------------------------------
global ASM_PFX(InternalX86RdRand64)
ASM_PFX(InternalX86RdRand64):
    rdrand rax                     ; generate a 64 bit RN into rax,
                                   ; CF=1 if RN generated ok, otherwise CF=0
    jc     rn64_ok                 ; jmp if CF=1
    xor    rax, rax                ; reg=0 if CF=0
    ret                            ; return with failure status
rn64_ok:
    mov    [rcx], rax
    mov    rax, 1
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
;    )
;------------------------------------------------------------------------------
global ASM_PFX(AsmCpuid)
ASM_PFX(AsmCpuid):
    push    rbx
    mov     eax, ecx
    push    rax                         ; save Index on stack
    push    rdx
    cpuid
    test    r9, r9
    jz      .0
    mov     [r9], ecx
.0:
    pop     rcx
    jrcxz   .1
    mov     [rcx], eax
.1:
    mov     rcx, r8
    jrcxz   .2
    mov     [rcx], ebx
.2:
    mov     rcx, [rsp + 0x38]
    jrcxz   .3
    mov     [rcx], edx
.3:
    pop     rax                         ; restore Index to rax as return value
    pop     rbx
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
    push    rbx
    mov     eax, ecx
    mov     ecx, edx
    push    rax                         ; save Index on stack
    cpuid
    mov     r10, [rsp + 0x38]
    test    r10, r10
    jz      .0
    mov     [r10], ecx
.0:
    mov     rcx, r8
    jrcxz   .1
    mov     [rcx], eax
.1:
    mov     rcx, r9
    jrcxz   .2
    mov     [rcx], ebx
.2:
    mov     rcx, [rsp + 0x40]
    jrcxz   .3
    mov     [rcx], edx
.3:
    pop     rax                         ; restore Index to rax as return value
    pop     rbx
    ret

;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; InternalX86ReadFsBase (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalX86ReadFsBase)
ASM_PFX(InternalX86ReadFsBase):
    rdfsbase rax
    ret

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; InternalX86WriteFsBase (
;   UINT64  FsBase
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalX86WriteFsBase)
ASM_PFX(InternalX86WriteFsBase):
    wrfsbase rcx
    ret
