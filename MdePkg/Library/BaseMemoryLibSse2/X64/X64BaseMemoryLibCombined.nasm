//------------------------------------------------------------------------------
// Combined X64 BaseMemoryLib Assembly Functions for BaseMemoryLibSse2
//------------------------------------------------------------------------------

    DEFAULT REL
    SECTION .text


;------------------------------------------------------------------------------
; INTN
; EFIAPI
; InternalMemCompareMem (
;   IN      CONST VOID                *DestinationBuffer,
;   IN      CONST VOID                *SourceBuffer,
;   IN      UINTN                     Length
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemCompareMem)
ASM_PFX(InternalMemCompareMem):
    push    rsi
    push    rdi
    mov     rsi, rcx
    mov     rdi, rdx
    mov     rcx, r8
    repe    cmpsb
    movzx   rax, byte [rsi - 1]
    movzx   rdx, byte [rdi - 1]
    sub     rax, rdx
    pop     rdi
    pop     rsi
    ret


;------------------------------------------------------------------------------
;  VOID *
;  EFIAPI
;  InternalMemCopyMem (
;    IN VOID   *Destination,
;    IN VOID   *Source,
;    IN UINTN  Count
;    );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemCopyMem)
ASM_PFX(InternalMemCopyMem):
    push    rsi
    push    rdi
    mov     rsi, rdx                    ; rsi <- Source
    mov     rdi, rcx                    ; rdi <- Destination
    lea     r9, [rsi + r8 - 1]          ; r9 <- Last byte of Source
    cmp     rsi, rdi
    mov     rax, rdi                    ; rax <- Destination as return value
    jae     .0                          ; Copy forward if Source > Destination
    cmp     r9, rdi                     ; Overlapped?
    jae     @CopyBackward               ; Copy backward if overlapped
.0:
    xor     rcx, rcx
    sub     rcx, rdi                    ; rcx <- -rdi
    and     rcx, 15                     ; rcx + rsi should be 16 bytes aligned
    jz      .1                          ; skip if rcx == 0
    cmp     rcx, r8
    cmova   rcx, r8
    sub     r8, rcx
    rep     movsb
.1:
    mov     rcx, r8
    and     r8, 15
    shr     rcx, 4                      ; rcx <- # of DQwords to copy
    jz      @CopyBytes
    movdqa  [rsp + 0x18], xmm0           ; save xmm0 on stack
.2:
    movdqu  xmm0, [rsi]                 ; rsi may not be 16-byte aligned
    movntdq [rdi], xmm0                 ; rdi should be 16-byte aligned
    add     rsi, 16
    add     rdi, 16
    loop    .2
    mfence
    movdqa  xmm0, [rsp + 0x18]           ; restore xmm0
    jmp     @CopyBytes                  ; copy remaining bytes
@CopyBackward:
    mov     rsi, r9                     ; rsi <- Last byte of Source
    lea     rdi, [rdi + r8 - 1]         ; rdi <- Last byte of Destination
    std
@CopyBytes:
    mov     rcx, r8
    rep     movsb
    cld
    pop     rdi
    pop     rsi
    ret


;------------------------------------------------------------------------------
;  BOOLEAN
;  EFIAPI
;  InternalMemIsZeroBuffer (
;    IN CONST VOID  *Buffer,
;    IN UINTN       Length
;    );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemIsZeroBuffer)
ASM_PFX(InternalMemIsZeroBuffer):
    push         rdi
    mov          rdi, rcx              ; rdi <- Buffer
    xor          rcx, rcx              ; rcx <- 0
    sub          rcx, rdi
    and          rcx, 15               ; rcx + rdi aligns on 16-byte boundary
    jz           @Is16BytesZero
    cmp          rcx, rdx              ; Length already in rdx
    cmova        rcx, rdx              ; bytes before the 16-byte boundary
    sub          rdx, rcx
    xor          rax, rax              ; rax <- 0, also set ZF
    repe         scasb
    jnz          @ReturnFalse          ; ZF=0 means non-zero element found
@Is16BytesZero:
    mov          rcx, rdx
    and          rdx, 15
    shr          rcx, 4
    jz           @IsBytesZero
.0:
    pxor         xmm0, xmm0            ; xmm0 <- 0
    pcmpeqb      xmm0, [rdi]           ; check zero for 16 bytes
    pmovmskb     eax, xmm0             ; eax <- compare results
                                       ; nasm doesn't support 64-bit destination
                                       ; for pmovmskb
    cmp          eax, 0xffff
    jnz          @ReturnFalse
    add          rdi, 16
    loop         .0
@IsBytesZero:
    mov          rcx, rdx
    xor          rax, rax              ; rax <- 0, also set ZF
    repe         scasb
    jnz          @ReturnFalse          ; ZF=0 means non-zero element found
    pop          rdi
    mov          rax, 1                ; return TRUE
    ret
@ReturnFalse:
    pop          rdi
    xor          rax, rax
    ret                                ; return FALSE


;------------------------------------------------------------------------------
; CONST VOID *
; EFIAPI
; InternalMemScanMem16 (
;   IN      CONST VOID                *Buffer,
;   IN      UINTN                     Length,
;   IN      UINT16                    Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemScanMem16)
ASM_PFX(InternalMemScanMem16):
    push    rdi
    mov     rdi, rcx
    mov     rax, r8
    mov     rcx, rdx
    repne   scasw
    lea     rax, [rdi - 2]
    cmovnz  rax, rcx
    pop     rdi
    ret


;------------------------------------------------------------------------------
; CONST VOID *
; EFIAPI
; InternalMemScanMem32 (
;   IN      CONST VOID                *Buffer,
;   IN      UINTN                     Length,
;   IN      UINT32                    Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemScanMem32)
ASM_PFX(InternalMemScanMem32):
    push    rdi
    mov     rdi, rcx
    mov     rax, r8
    mov     rcx, rdx
    repne   scasd
    lea     rax, [rdi - 4]
    cmovnz  rax, rcx
    pop     rdi
    ret


;------------------------------------------------------------------------------
; CONST VOID *
; EFIAPI
; InternalMemScanMem64 (
;   IN      CONST VOID                *Buffer,
;   IN      UINTN                     Length,
;   IN      UINT64                    Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemScanMem64)
ASM_PFX(InternalMemScanMem64):
    push    rdi
    mov     rdi, rcx
    mov     rax, r8
    mov     rcx, rdx
    repne   scasq
    lea     rax, [rdi - 8]
    cmovnz  rax, rcx
    pop     rdi
    ret


;------------------------------------------------------------------------------
; CONST VOID *
; EFIAPI
; InternalMemScanMem8 (
;   IN      CONST VOID                *Buffer,
;   IN      UINTN                     Length,
;   IN      UINT8                     Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemScanMem8)
ASM_PFX(InternalMemScanMem8):
    push    rdi
    mov     rdi, rcx
    mov     rcx, rdx
    mov     rax, r8
    repne   scasb
    lea     rax, [rdi - 1]
    cmovnz  rax, rcx                    ; set rax to 0 if not found
    pop     rdi
    ret


;------------------------------------------------------------------------------
;  VOID *
;  InternalMemSetMem (
;    IN VOID   *Buffer,
;    IN UINTN  Count,
;    IN UINT8  Value
;    )
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemSetMem)
ASM_PFX(InternalMemSetMem):
    push    rdi
    mov     rdi, rcx                    ; rdi <- Buffer
    mov     al, r8b                     ; al <- Value
    mov     r9, rdi                     ; r9 <- Buffer as return value
    xor     rcx, rcx
    sub     rcx, rdi
    and     rcx, 15                     ; rcx + rdi aligns on 16-byte boundary
    jz      .0
    cmp     rcx, rdx
    cmova   rcx, rdx
    sub     rdx, rcx
    rep     stosb
.0:
    mov     rcx, rdx
    and     rdx, 63
    shr     rcx, 6
    jz      @SetBytes
    mov     ah, al                      ; ax <- Value repeats twice
    movdqa  [rsp + 0x10], xmm0           ; save xmm0
    movd    xmm0, eax                   ; xmm0[0..16] <- Value repeats twice
    pshuflw xmm0, xmm0, 0               ; xmm0[0..63] <- Value repeats 8 times
    movlhps xmm0, xmm0                  ; xmm0 <- Value repeats 16 times
.1:
    movntdq [rdi], xmm0                 ; rdi should be 16-byte aligned
    movntdq [rdi + 16], xmm0
    movntdq [rdi + 32], xmm0
    movntdq [rdi + 48], xmm0
    add     rdi, 64
    loop    .1
    mfence
    movdqa  xmm0, [rsp + 0x10]           ; restore xmm0
@SetBytes:
    mov     ecx, edx                    ; high 32 bits of rcx are always zero
    rep     stosb
    mov     rax, r9                     ; rax <- Return value
    pop     rdi
    ret


;------------------------------------------------------------------------------
;  VOID *
;  InternalMemSetMem16 (
;    IN VOID   *Buffer,
;    IN UINTN  Count,
;    IN UINT16 Value
;    )
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemSetMem16)
ASM_PFX(InternalMemSetMem16):
    push    rdi
    mov     rdi, rcx
    mov     r9, rdi
    xor     rcx, rcx
    sub     rcx, rdi
    and     rcx, 63
    mov     rax, r8
    jz      .0
    shr     rcx, 1
    cmp     rcx, rdx
    cmova   rcx, rdx
    sub     rdx, rcx
    rep     stosw
.0:
    mov     rcx, rdx
    and     edx, 31
    shr     rcx, 5
    jz      @SetWords
    movd    xmm0, eax
    pshuflw xmm0, xmm0, 0
    movlhps xmm0, xmm0
.1:
    movntdq [rdi], xmm0
    movntdq [rdi + 16], xmm0
    movntdq [rdi + 32], xmm0
    movntdq [rdi + 48], xmm0
    add     rdi, 64
    loop    .1
    mfence
@SetWords:
    mov     ecx, edx
    rep     stosw
    mov     rax, r9
    pop     rdi
    ret


;------------------------------------------------------------------------------
;  VOID *
;  InternalMemSetMem32 (
;    IN VOID   *Buffer,
;    IN UINTN  Count,
;    IN UINT8  Value
;    )
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemSetMem32)
ASM_PFX(InternalMemSetMem32):
    push    rdi
    mov     rdi, rcx
    mov     r9, rdi
    xor     rcx, rcx
    sub     rcx, rdi
    and     rcx, 15
    mov     rax, r8
    jz      .0
    shr     rcx, 2
    cmp     rcx, rdx
    cmova   rcx, rdx
    sub     rdx, rcx
    rep     stosd
.0:
    mov     rcx, rdx
    and     edx, 15
    shr     rcx, 4
    jz      @SetDwords
    movd    xmm0, eax
    pshufd  xmm0, xmm0, 0
.1:
    movntdq [rdi], xmm0
    movntdq [rdi + 16], xmm0
    movntdq [rdi + 32], xmm0
    movntdq [rdi + 48], xmm0
    add     rdi, 64
    loop    .1
    mfence
@SetDwords:
    mov     ecx, edx
    rep     stosd
    mov     rax, r9
    pop     rdi
    ret


;------------------------------------------------------------------------------
;  VOID *
;  InternalMemSetMem64 (
;    IN VOID   *Buffer,
;    IN UINTN  Count,
;    IN UINT64 Value
;    )
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemSetMem64)
ASM_PFX(InternalMemSetMem64):
    mov     rax, rcx                    ; rax <- Buffer
    xchg    rcx, rdx                    ; rcx <- Count & rdx <- Buffer
    test    dl, 8
    movq    xmm0, r8
    jz      .0
    mov     [rdx], r8
    add     rdx, 8
    dec     rcx
.0:
    push    rbx
    mov     rbx, rcx
    and     rbx, 7
    shr     rcx, 3
    jz      @SetQwords
    movlhps xmm0, xmm0
.1:
    movntdq [rdx], xmm0
    movntdq [rdx + 16], xmm0
    movntdq [rdx + 32], xmm0
    movntdq [rdx + 48], xmm0
    lea     rdx, [rdx + 64]
    loop    .1
    mfence
@SetQwords:
    push    rdi
    mov     rcx, rbx
    mov     rax, r8
    mov     rdi, rdx
    rep     stosq
    pop     rdi
.2:
    pop rbx
    ret


;------------------------------------------------------------------------------
;  VOID *
;  InternalMemZeroMem (
;    IN VOID   *Buffer,
;    IN UINTN  Count
;    )
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemZeroMem)
ASM_PFX(InternalMemZeroMem):
    push    rdi
    mov     rdi, rcx
    xor     rcx, rcx
    xor     eax, eax
    sub     rcx, rdi
    and     rcx, 63
    mov     r8, rdi
    jz      .0
    cmp     rcx, rdx
    cmova   rcx, rdx
    sub     rdx, rcx
    rep     stosb
.0:
    mov     rcx, rdx
    and     edx, 63
    shr     rcx, 6
    jz      @ZeroBytes
    pxor    xmm0, xmm0
.1:
    movntdq [rdi], xmm0
    movntdq [rdi + 16], xmm0
    movntdq [rdi + 32], xmm0
    movntdq [rdi + 48], xmm0
    add     rdi, 64
    loop    .1
    mfence
@ZeroBytes:
    mov     ecx, edx
    rep     stosb
    mov     rax, r8
    pop     rdi
    ret

