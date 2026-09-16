//------------------------------------------------------------------------------
// Combined X64 NASM Virtualization Instructions
//------------------------------------------------------------------------------
#include "BaseLibInternals.h"

    DEFAULT REL
    SECTION .text


%macro tdcall 0
    db 0x66,0x0f,0x01,0xcc
%endmacro

%macro tdcall_push_regs 0
    push rbp
    mov  rbp, rsp
    push r15
    push r14
    push r13
    push r12
    push rbx
    push rsi
    push rdi
%endmacro

%macro tdcall_pop_regs 0
    pop rdi
    pop rsi
    pop rbx
    pop r12
    pop r13
    pop r14
    pop r15
    pop rbp
%endmacro

%define number_of_regs_pushed 8
%define number_of_parameters  4

;
; Keep these in sync for push_regs/pop_regs, code below
; uses them to find 5th or greater parameters
;
%define first_variable_on_stack_offset \
  ((number_of_regs_pushed * 8) + (number_of_parameters * 8) + 8)
%define second_variable_on_stack_offset \
  ((first_variable_on_stack_offset) + 8)

;  TdCall (
;    UINT64  Leaf,    // Rcx
;    UINT64  P1,      // Rdx
;    UINT64  P2,      // R8
;    UINT64  P3,      // R9
;    UINT64  Results, // rsp + 0x28
;    )
global ASM_PFX(TdCall)
ASM_PFX(TdCall):
       tdcall_push_regs

       mov rax, rcx
       mov rcx, rdx
       mov rdx, r8
       mov r8, r9

       tdcall

       ; exit if tdcall reports failure.
       test rax, rax
       jnz .exit

       ; test if caller wanted results
       mov r12, [rsp + first_variable_on_stack_offset ]
       test r12, r12
       jz .exit
       mov [r12 + 0 ], rcx
       mov [r12 + 8 ], rdx
       mov [r12 + 16], r8
       mov [r12 + 24], r9
       mov [r12 + 32], r10
       mov [r12 + 40], r11
.exit:
       tdcall_pop_regs
       ret

%define TDVMCALL_EXPOSE_REGS_MASK       0xffcc
%define TDVMCALL                        0x0




%define number_of_regs_pushed 8
%define number_of_parameters  4

;
; Keep these in sync for push_regs/pop_regs, code below
; uses them to find 5th or greater parameters
;
%define first_variable_on_stack_offset \
  ((number_of_regs_pushed * 8) + (number_of_parameters * 8) + 8)
%define second_variable_on_stack_offset \
  ((first_variable_on_stack_offset) + 8)

%macro tdcall_regs_preamble 2
    mov rax, %1

    xor rcx, rcx
    mov ecx, %2

    ; R10 = 0 (standard TDVMCALL)

    xor r10d, r10d

    ; Zero out unused (for standard TDVMCALL) registers to avoid leaking
    ; secrets to the VMM.

    xor ebx, ebx
    xor esi, esi
    xor edi, edi

    xor edx, edx
    xor ebp, ebp
    xor r8d, r8d
    xor r9d, r9d
%endmacro

%macro tdcall_regs_postamble 0
    xor ebx, ebx
    xor esi, esi
    xor edi, edi

    xor ecx, ecx
    xor edx, edx
    xor r8d,  r8d
    xor r9d,  r9d
    xor r10d, r10d
    xor r11d, r11d
%endmacro

;------------------------------------------------------------------------------
; 0   => RAX = TDCALL leaf
; M   => RCX = TDVMCALL register behavior
; 1   => R10 = standard vs. vendor
; RDI => R11 = TDVMCALL function / nr
; RSI =  R12 = p1
; RDX => R13 = p2
; RCX => R14 = p3
; R8  => R15 = p4

;  UINT64
;  EFIAPI
;  TdVmCall (
;    UINT64  Leaf,  // Rcx
;    UINT64  P1,  // Rdx
;    UINT64  P2,  // R8
;    UINT64  P3,  // R9
;    UINT64  P4,  // rsp + 0x28
;    UINT64  *Val // rsp + 0x30
;    )
global ASM_PFX(TdVmCall)
ASM_PFX(TdVmCall):
       tdcall_push_regs

       mov r11, rcx
       mov r12, rdx
       mov r13, r8
       mov r14, r9
       mov r15, [rsp + first_variable_on_stack_offset ]

       tdcall_regs_preamble TDVMCALL, TDVMCALL_EXPOSE_REGS_MASK

       tdcall

       ; ignore return data if TDCALL reports failure.
       test rax, rax
       jnz .no_return_data

       ; Propagate TDVMCALL success/failure to return value.
       mov rax, r10

       ; Retrieve the Val pointer.
       mov r9, [rsp + second_variable_on_stack_offset ]
       test r9, r9
       jz .no_return_data

       ; Propagate TDVMCALL output value to output param
       mov [r9], r11
.no_return_data:
       tdcall_regs_postamble

       tdcall_pop_regs

       ret


;-----------------------------------------------------------------------------
;  UINT32
;  EFIAPI
;  AsmPvalidate (
;    IN   UINT32              PageSize
;    IN   UINT32              Validate,
;    IN   UINT64              Address
;    )
;-----------------------------------------------------------------------------
global ASM_PFX(AsmPvalidate)
ASM_PFX(AsmPvalidate):
  mov     rax, r8

  PVALIDATE

  ; Save the carry flag.
  setc    dl

  ; The PVALIDATE instruction returns the status in rax register.
  cmp     rax, 0
  jne     PvalidateExit

  ; Check the carry flag to determine if RMP entry was updated.
  cmp     dl, 0
  je      PvalidateExit

  ; Return the PVALIDATE_RET_NO_RMPUPDATE.
  mov     rax, 255

PvalidateExit:
  ret


;-----------------------------------------------------------------------------
;  UINT32
;  EFIAPI
;  AsmRmpAdjust (
;    IN  UINT64  Rax,
;    IN  UINT64  Rcx,
;    IN  UINT64  Rdx
;    )
;-----------------------------------------------------------------------------
global ASM_PFX(AsmRmpAdjust)
ASM_PFX(AsmRmpAdjust):
  mov     rax, rcx       ; Input Rax is in RCX by calling convention
  mov     rcx, rdx       ; Input Rcx is in RDX by calling convention
  mov     rdx, r8        ; Input Rdx is in R8  by calling convention

  RMPADJUST

  ; RMPADJUST returns the status in the EAX register.
  ret

;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmXGetBv (
;   IN UINT32  Index
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmXGetBv)
ASM_PFX(AsmXGetBv):
    xgetbv
    shl     rdx, 32
    or      rax, rdx
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
    mov     rax, rdx                    ; meanwhile, rax <- return value
    shr     rdx, 0x20                    ; edx:eax contains the value to write
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
    rep     vmmcall
    ret


;------------------------------------------------------------------------------
; typedef struct {
;   VOID      *Caa;
;   UINT64    RaxIn;
;   UINT64    RcxIn;
;   UINT64    RdxIn;
;   UINT64    R8In;
;   UINT64    R9In;
;   UINT64    RaxOut;
;   UINT64    RcxOut;
;   UINT64    RdxOut;
;   UINT64    R8Out;
;   UINT64    R9Out;
;   UINT8     *CallPending;
; } SVSM_CALL_DATA;
;
; UINT32
; EFIAPI
; AsmVmgExitSvsm (
;   SVSM_CALL_DATA *SvsmCallData
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmVmgExitSvsm)
ASM_PFX(AsmVmgExitSvsm):
    push    r10
    push    r11
    push    r12

;
; Calling convention has SvsmCallData in RCX. Move RCX to R12 in order to
; properly populate the SVSM register state.
;
    mov     r12, rcx

    mov     rax, [r12 + 8]
    mov     rcx, [r12 + 16]
    mov     rdx, [r12 + 24]
    mov     r8,  [r12 + 32]
    mov     r9,  [r12 + 40]

;
; Set CA call pending
;
    mov     r10, [r12]
    mov     byte [r10], 1

    rep     vmmcall

    mov     [r12 + 48], rax
    mov     [r12 + 56], rcx
    mov     [r12 + 64], rdx
    mov     [r12 + 72], r8
    mov     [r12 + 80], r9

;
; Perform the atomic exchange and return the CA call pending value.
; The call pending value is a one-byte field at offset 0 into the CA,
; which is currently the value in R10.
;

    mov     r11, [r12 + 88]     ; Get CallPending address
    mov     cl, byte [r11]
    xchg    byte [r10], cl
    mov     byte [r11], cl      ; Return the exchanged value

    pop     r12
    pop     r11
    pop     r10

;
; RAX has the value to be returned from the SVSM
;
    ret

