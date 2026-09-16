//------------------------------------------------------------------------------
// Combined Ia32 BaseMemoryLib Assembly Functions for BaseMemoryLibSse2
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
    push    esi
    push    edi
    mov     esi, [esp + 12]
    mov     edi, [esp + 16]
    mov     ecx, [esp + 20]
    repe    cmpsb
    movzx   eax, byte [esi - 1]
    movzx   edx, byte [edi - 1]
    sub     eax, edx
    pop     edi
    pop     esi
    ret


;------------------------------------------------------------------------------
;  VOID *
;  InternalMemCopyMem (
;    IN VOID   *Destination,
;    IN VOID   *Source,
;    IN UINTN  Count
;    );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemCopyMem)
ASM_PFX(InternalMemCopyMem):
    push    esi
    push    edi
    mov     esi, [esp + 16]             ; esi <- Source
    mov     edi, [esp + 12]             ; edi <- Destination
    mov     edx, [esp + 20]             ; edx <- Count
    lea     eax, [esi + edx - 1]        ; eax <- End of Source
    cmp     esi, edi
    jae     .0
    cmp     eax, edi                    ; Overlapped?
    jae     @CopyBackward               ; Copy backward if overlapped
.0:
    xor     ecx, ecx
    sub     ecx, edi
    and     ecx, 15                     ; ecx + edi aligns on 16-byte boundary
    jz      .1
    cmp     ecx, edx
    cmova   ecx, edx
    sub     edx, ecx                    ; edx <- remaining bytes to copy
    rep     movsb
.1:
    mov     ecx, edx
    and     edx, 15
    shr     ecx, 4                      ; ecx <- # of DQwords to copy
    jz      @CopyBytes
    add     esp, -16
    movdqu  [esp], xmm0                 ; save xmm0
.2:
    movdqu  xmm0, [esi]                 ; esi may not be 16-bytes aligned
    movntdq [edi], xmm0                 ; edi should be 16-bytes aligned
    add     esi, 16
    add     edi, 16
    loop    .2
    mfence
    movdqu  xmm0, [esp]                 ; restore xmm0
    add     esp, 16                     ; stack cleanup
    jmp     @CopyBytes
@CopyBackward:
    mov     esi, eax                    ; esi <- Last byte in Source
    lea     edi, [edi + edx - 1]        ; edi <- Last byte in Destination
    std
@CopyBytes:
    mov     ecx, edx
    rep     movsb
    cld
    mov     eax, [esp + 12]             ; eax <- Destination as return value
    pop     edi
    pop     esi
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
    push         edi
    mov          edi, [esp + 8]        ; edi <- Buffer
    mov          edx, [esp + 12]       ; edx <- Length
    xor          ecx, ecx              ; ecx <- 0
    sub          ecx, edi
    and          ecx, 15               ; ecx + edi aligns on 16-byte boundary
    jz           @Is16BytesZero
    cmp          ecx, edx
    cmova        ecx, edx              ; bytes before the 16-byte boundary
    sub          edx, ecx
    xor          eax, eax              ; eax <- 0, also set ZF
    repe         scasb
    jnz          @ReturnFalse          ; ZF=0 means non-zero element found
@Is16BytesZero:
    mov          ecx, edx
    and          edx, 15
    shr          ecx, 4
    jz           @IsBytesZero
.0:
    pxor         xmm0, xmm0            ; xmm0 <- 0
    pcmpeqb      xmm0, [edi]           ; check zero for 16 bytes
    pmovmskb     eax, xmm0             ; eax <- compare results
    cmp          eax, 0xffff
    jnz          @ReturnFalse
    add          edi, 16
    loop         .0
@IsBytesZero:
    mov          ecx, edx
    xor          eax, eax              ; eax <- 0, also set ZF
    repe         scasb
    jnz          @ReturnFalse          ; ZF=0 means non-zero element found
    pop          edi
    mov          eax, 1                ; return TRUE
    ret
@ReturnFalse:
    pop          edi
    xor          eax, eax
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
    push    edi
    mov     ecx, [esp + 12]
    mov     edi, [esp + 8]
    mov     eax, [esp + 16]
    repne   scasw
    lea     eax, [edi - 2]
    cmovnz  eax, ecx
    pop     edi
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
    push    edi
    mov     ecx, [esp + 12]
    mov     edi, [esp + 8]
    mov     eax, [esp + 16]
    repne   scasd
    lea     eax, [edi - 4]
    cmovnz  eax, ecx
    pop     edi
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
    push    edi
    mov     ecx, [esp + 12]
    mov     eax, [esp + 16]
    mov     edx, [esp + 20]
    mov     edi, [esp + 8]
.0:
    cmp     eax, [edi]
    lea     edi, [edi + 8]
    loopne  .0
    jne     .1
    cmp     edx, [edi - 4]
    jecxz   .1
    jne     .0
.1:
    lea     eax, [edi - 8]
    cmovne  eax, ecx
    pop     edi
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
    push    edi
    mov     ecx, [esp + 12]
    mov     edi, [esp + 8]
    mov     al, [esp + 16]
    repne   scasb
    lea     eax, [edi - 1]
    cmovnz  eax, ecx
    pop     edi
    ret


;------------------------------------------------------------------------------
;  VOID *
;  EFIAPI
;  InternalMemSetMem (
;    IN VOID   *Buffer,
;    IN UINTN  Count,
;    IN UINT8  Value
;    );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemSetMem)
ASM_PFX(InternalMemSetMem):
    push    edi
    mov     edx, [esp + 12]             ; edx <- Count
    mov     edi, [esp + 8]              ; edi <- Buffer
    mov     al, [esp + 16]              ; al <- Value
    xor     ecx, ecx
    sub     ecx, edi
    and     ecx, 63                     ; ecx + edi aligns on 16-byte boundary
    jz      .0
    cmp     ecx, edx
    cmova   ecx, edx
    sub     edx, ecx
    rep     stosb
.0:
    mov     ecx, edx
    and     edx, 63
    shr     ecx, 6                      ; ecx <- # of DQwords to set
    jz      @SetBytes
    mov     ah, al                      ; ax <- Value | (Value << 8)
    add     esp, -16
    movdqu  [esp], xmm0                 ; save xmm0
    movd    xmm0, eax
    pshuflw xmm0, xmm0, 0               ; xmm0[0..63] <- Value repeats 8 times
    movlhps xmm0, xmm0                  ; xmm0 <- Value repeats 16 times
.1:
    movntdq [edi], xmm0                 ; edi should be 16-byte aligned
    movntdq [edi + 16], xmm0
    movntdq [edi + 32], xmm0
    movntdq [edi + 48], xmm0
    add     edi, 64
    loop    .1
    mfence
    movdqu  xmm0, [esp]                 ; restore xmm0
    add     esp, 16                     ; stack cleanup
@SetBytes:
    mov     ecx, edx
    rep     stosb
    mov     eax, [esp + 8]              ; eax <- Buffer as return value
    pop     edi
    ret


;------------------------------------------------------------------------------
;  VOID *
;  EFIAPI
;  InternalMemSetMem16 (
;    IN VOID   *Buffer,
;    IN UINTN  Count,
;    IN UINT16 Value
;    );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemSetMem16)
ASM_PFX(InternalMemSetMem16):
    push    edi
    mov     edx, [esp + 12]
    mov     edi, [esp + 8]
    xor     ecx, ecx
    sub     ecx, edi
    and     ecx, 63                     ; ecx + edi aligns on 16-byte boundary
    mov     eax, [esp + 16]
    jz      .0
    shr     ecx, 1
    cmp     ecx, edx
    cmova   ecx, edx
    sub     edx, ecx
    rep     stosw
.0:
    mov     ecx, edx
    and     edx, 31
    shr     ecx, 5
    jz      @SetWords
    movd    xmm0, eax
    pshuflw xmm0, xmm0, 0
    movlhps xmm0, xmm0
.1:
    movntdq [edi], xmm0                 ; edi should be 16-byte aligned
    movntdq [edi + 16], xmm0
    movntdq [edi + 32], xmm0
    movntdq [edi + 48], xmm0
    add     edi, 64
    loop    .1
    mfence
@SetWords:
    mov     ecx, edx
    rep     stosw
    mov     eax, [esp + 8]
    pop     edi
    ret


;------------------------------------------------------------------------------
;  VOID *
;  EFIAPI
;  InternalMemSetMem32 (
;    IN VOID   *Buffer,
;    IN UINTN  Count,
;    IN UINT32 Value
;    );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemSetMem32)
ASM_PFX(InternalMemSetMem32):
    push    edi
    mov     edx, [esp + 12]
    mov     edi, [esp + 8]
    xor     ecx, ecx
    sub     ecx, edi
    and     ecx, 15                     ; ecx + edi aligns on 16-byte boundary
    mov     eax, [esp + 16]
    jz      .0
    shr     ecx, 2
    cmp     ecx, edx
    cmova   ecx, edx
    sub     edx, ecx
    rep     stosd
.0:
    mov     ecx, edx
    and     edx, 15
    shr     ecx, 4
    jz      @SetDwords
    movd    xmm0, eax
    pshufd  xmm0, xmm0, 0
.1:
    movntdq [edi], xmm0
    movntdq [edi + 16], xmm0
    movntdq [edi + 32], xmm0
    movntdq [edi + 48], xmm0
    add     edi, 64
    loop    .1
    mfence
@SetDwords:
    mov     ecx, edx
    rep     stosd
    mov     eax, [esp + 8]
    pop     edi
    ret


;------------------------------------------------------------------------------
;  VOID *
;  EFIAPI
;  InternalMemSetMem64 (
;    IN VOID   *Buffer,
;    IN UINTN  Count,
;    IN UINT64 Value
;    )
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemSetMem64)
ASM_PFX(InternalMemSetMem64):
    mov     eax, [esp + 4]              ; eax <- Buffer
    mov     ecx, [esp + 8]              ; ecx <- Count
    test    al, 8
    mov     edx, eax
    movq    xmm0, qword [esp + 12]
    jz      .0
    movq    qword [edx], xmm0
    add     edx, 8
    dec     ecx
.0:
    push    ebx
    mov     ebx, ecx
    and     ebx, 7
    shr     ecx, 3
    jz      @SetQwords
    movlhps xmm0, xmm0
.1:
    movntdq [edx], xmm0
    movntdq [edx + 16], xmm0
    movntdq [edx + 32], xmm0
    movntdq [edx + 48], xmm0
    lea     edx, [edx + 64]
    loop    .1
    mfence
@SetQwords:
    test    ebx, ebx
    jz .3
    mov     ecx, ebx
.2
    movq    qword [edx], xmm0
    lea     edx, [edx + 8]
    loop    .2
.3:
    pop ebx
    ret


;------------------------------------------------------------------------------
;  VOID *
;  EFIAPI
;  InternalMemZeroMem (
;    IN VOID   *Buffer,
;    IN UINTN  Count
;    );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemZeroMem)
ASM_PFX(InternalMemZeroMem):
    push    edi
    mov     edi, [esp + 8]
    mov     edx, [esp + 12]
    xor     ecx, ecx
    sub     ecx, edi
    xor     eax, eax
    and     ecx, 63
    jz      .0
    cmp     ecx, edx
    cmova   ecx, edx
    sub     edx, ecx
    rep     stosb
.0:
    mov     ecx, edx
    and     edx, 63
    shr     ecx, 6
    jz      @ZeroBytes
    pxor    xmm0, xmm0
.1:
    movntdq [edi], xmm0
    movntdq [edi + 16], xmm0
    movntdq [edi + 32], xmm0
    movntdq [edi + 48], xmm0
    add     edi, 64
    loop    .1
    mfence
@ZeroBytes:
    mov     ecx, edx
    rep     stosb
    mov     eax, [esp + 8]
    pop     edi
    ret

