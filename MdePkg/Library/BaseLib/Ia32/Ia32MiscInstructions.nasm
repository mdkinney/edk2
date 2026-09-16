//------------------------------------------------------------------------------
// Combined Ia32 NASM Misc Instructions
//------------------------------------------------------------------------------
#include "BaseLibInternals.h"

    DEFAULT REL
    SECTION .text


;------------------------------------------------------------------------------
; VOID
; AsmWriteTr (
;   UINT16 Selector
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmWriteTr)
ASM_PFX(AsmWriteTr):
    mov     eax, [esp+4]
    ltr     ax
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

extern ASM_PFX(InternalMathDivRemU64x32)

;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; InternalMathDivRemU64x64 (
;   IN      UINT64                    Dividend,
;   IN      UINT64                    Divisor,
;   OUT     UINT64                    *Remainder    OPTIONAL
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMathDivRemU64x64)
ASM_PFX(InternalMathDivRemU64x64):
    mov     ecx, [esp + 16]             ; ecx <- divisor[32..63]
    test    ecx, ecx
    jnz     _@DivRemU64x64              ; call _@DivRemU64x64 if Divisor > 2^32
    mov     ecx, [esp + 20]
    jecxz   .0
    and     dword [ecx + 4], 0      ; zero high dword of remainder
    mov     [esp + 16], ecx             ; set up stack frame to match DivRemU64x32
.0:
    jmp     ASM_PFX(InternalMathDivRemU64x32)

_@DivRemU64x64:
    push    ebx
    push    esi
    push    edi
    mov     edx, dword [esp + 20]
    mov     eax, dword [esp + 16]   ; edx:eax <- dividend
    mov     edi, edx
    mov     esi, eax                    ; edi:esi <- dividend
    mov     ebx, dword [esp + 24]   ; ecx:ebx <- divisor
.1:
    shr     edx, 1
    rcr     eax, 1
    shrd    ebx, ecx, 1
    shr     ecx, 1
    jnz     .1
    div     ebx
    mov     ebx, eax                    ; ebx <- quotient
    mov     ecx, [esp + 28]             ; ecx <- high dword of divisor
    mul     dword [esp + 24]        ; edx:eax <- quotient * divisor[0..31]
    imul    ecx, ebx                    ; ecx <- quotient * divisor[32..63]
    add     edx, ecx                    ; edx <- (quotient * divisor)[32..63]
    mov     ecx, dword [esp + 32]   ; ecx <- addr for Remainder
    jc      @TooLarge                   ; product > 2^64
    cmp     edi, edx                    ; compare high 32 bits
    ja      @Correct
    jb      @TooLarge                   ; product > dividend
    cmp     esi, eax
    jae     @Correct                    ; product <= dividend
@TooLarge:
    dec     ebx                         ; adjust quotient by -1
    jecxz   @Return                     ; return if Remainder == NULL
    sub     eax, dword [esp + 24]
    sbb     edx, dword [esp + 28]   ; edx:eax <- (quotient - 1) * divisor
@Correct:
    jecxz   @Return
    sub     esi, eax
    sbb     edi, edx                    ; edi:esi <- remainder
    mov     [ecx], esi
    mov     [ecx + 4], edi
@Return:
    mov     eax, ebx                    ; eax <- quotient
    xor     edx, edx                    ; quotient is 32 bits long
    pop     edi
    pop     esi
    pop     ebx
    ret


;------------------------------------------------------------------------------
;  Generates a 16 bit random number through RDRAND instruction.
;  Return TRUE if Rand generated successfully, or FALSE if not.
;
;  BOOLEAN EFIAPI InternalX86RdRand16 (UINT16 *Rand);
;------------------------------------------------------------------------------
global ASM_PFX(InternalX86RdRand16)
ASM_PFX(InternalX86RdRand16):
    rdrand eax                     ; generate a 16 bit RN into ax
                                   ; CF=1 if RN generated ok, otherwise CF=0
    jc     rn16_ok                 ; jmp if CF=1
    xor    eax, eax                ; reg=0 if CF=0
    ret                            ; return with failure status
rn16_ok:
    mov    edx, dword [esp + 4]
    mov    [edx], ax
    mov    eax,  1
    ret

;------------------------------------------------------------------------------
;  Generates a 32 bit random number through RDRAND instruction.
;  Return TRUE if Rand generated successfully, or FALSE if not.
;
;  BOOLEAN EFIAPI InternalX86RdRand32 (UINT32 *Rand);
;------------------------------------------------------------------------------
global ASM_PFX(InternalX86RdRand32)
ASM_PFX(InternalX86RdRand32):
    rdrand eax                     ; generate a 32 bit RN into eax
                                   ; CF=1 if RN generated ok, otherwise CF=0
    jc     rn32_ok                 ; jmp if CF=1
    xor    eax, eax                ; reg=0 if CF=0
    ret                            ; return with failure status
rn32_ok:
    mov    edx, dword [esp + 4]
    mov    [edx], eax
    mov    eax,  1
    ret

;------------------------------------------------------------------------------
;  Generates a 64 bit random number through RDRAND instruction.
;  Return TRUE if Rand generated successfully, or FALSE if not.
;
;  BOOLEAN EFIAPI InternalX86RdRand64 (UINT64 *Rand);
;------------------------------------------------------------------------------
global ASM_PFX(InternalX86RdRand64)
ASM_PFX(InternalX86RdRand64):
    rdrand eax                     ; generate a 32 bit RN into eax
                                   ; CF=1 if RN generated ok, otherwise CF=0
    jnc    rn64_ret                ; jmp if CF=0
    mov    edx, dword [esp + 4]
    mov    [edx], eax

    rdrand eax                     ; generate another 32 bit RN
    jnc    rn64_ret                ; jmp if CF=0
    mov    [edx + 4], eax

    mov    eax,  1
    ret
rn64_ret:
    xor    eax, eax
    ret                            ; return with failure status


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmXGetBv (
;   IN UINT32  Index
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmXGetBv)
ASM_PFX(AsmXGetBv):
    mov     ecx, [esp + 4]
    xgetbv
    ret

;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmXSetBv (
;   IN UINT32  Index,
;   IN UINT64  Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmXSetBv)
ASM_PFX(AsmXSetBv):
    mov     edx, [esp + 12]
    mov     eax, [esp + 8]
    mov     ecx, [esp + 4]
    xsetbv
    ret

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmVmgExit (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmVmgExit)
ASM_PFX(AsmVmgExit):
;
; NASM doesn't support the vmmcall instruction in 32-bit mode and NASM versions
; before 2.12 cannot translate the 64-bit "rep vmmcall" instruction into elf32
; format. Given that VMGEXIT does not make sense on IA32, provide a stub
; implementation that is identical to CpuBreakpoint(). In practice, AsmVmgExit()
; should never be called on IA32.
;
    int  3
    ret


;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; AsmVmgExitSvsm (
;   SVSM_CALL_DATA *SvsmCallData
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmVmgExitSvsm)
ASM_PFX(AsmVmgExitSvsm):
;
; NASM doesn't support the vmmcall instruction in 32-bit mode and NASM versions
; before 2.12 cannot translate the 64-bit "rep vmmcall" instruction into elf32
; format. Given that VMGEXIT does not make sense on IA32, provide a stub
; implementation that is identical to CpuBreakpoint(). In practice,
; AsmVmgExitSvsm() should never be called on IA32.
;
    int  3
    ret

