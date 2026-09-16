//------------------------------------------------------------------------------
// Combined X64 BaseMemoryLib Assembly Functions for BaseMemoryLibMmx
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
; VOID *
; EFIAPI
; InternalMemCopyMem (
;   OUT     VOID                      *DestinationBuffer,
;   IN      CONST VOID                *SourceBuffer,
;   IN      UINTN                     Length
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemCopyMem)
ASM_PFX(InternalMemCopyMem):
    push    rsi
    push    rdi
    mov     rsi, rdx                    ; rsi <- Source
    mov     rdi, rcx                    ; rdi <- Destination
    lea     r9, [rsi + r8 - 1]          ; r9 <- End of Source
    cmp     rsi, rdi
    mov     rax, rdi                    ; rax <- Destination as return value
    jae     .0
    cmp     r9, rdi
    jae     @CopyBackward               ; Copy backward if overlapped
.0:
    mov     rcx, r8
    and     r8, 7
    shr     rcx, 3                      ; rcx <- # of Qwords to copy
    jz      @CopyBytes
    movq    r10, mm0
.1:
    movq    mm0, [rsi]
    movntq  [rdi], mm0
    add     rsi, 8
    add     rdi, 8
    loop    .1
    mfence
    movq    mm0, r10
    jmp     @CopyBytes
@CopyBackward:
    mov     rsi, r9                     ; rsi <- End of Source
    lea     rdi, [rdi + r8 - 1]         ; rdi <- End of Destination
    std                                 ; set direction flag
@CopyBytes:
    mov     rcx, r8
    rep     movsb                       ; Copy bytes backward
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
    push    rdi
    mov     rdi, rcx                   ; rdi <- Buffer
    mov     rcx, rdx                   ; rcx <- Length
    shr     rcx, 3                     ; rcx <- number of qwords
    and     rdx, 7                     ; rdx <- number of trailing bytes
    xor     rax, rax                   ; rax <- 0, also set ZF
    repe    scasq
    jnz     @ReturnFalse               ; ZF=0 means non-zero element found
    mov     rcx, rdx
    repe    scasb
    jnz     @ReturnFalse
    pop     rdi
    mov     rax, 1                     ; return TRUE
    ret
@ReturnFalse:
    pop     rdi
    xor     rax, rax
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
; VOID *
; EFIAPI
; InternalMemSetMem (
;   OUT     VOID                      *Buffer,
;   IN      UINTN                     Length,
;   IN      UINT8                     Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemSetMem)
ASM_PFX(InternalMemSetMem):
    push    rdi
    mov     rax, r8
    mov     ah, al
    movq    mm0, rax
    mov     r8, rcx
    mov     rdi, r8                     ; rdi <- Buffer
    mov     rcx, rdx
    and     edx, 7
    shr     rcx, 3
    jz      @SetBytes
    pshufw  mm0, mm0, 0
.0:
    movntq  [rdi], mm0
    add     rdi, 8
    loop    .0
    mfence
@SetBytes:
    mov     ecx, edx
    rep     stosb
    mov     rax, r8
    pop     rdi
    ret


;------------------------------------------------------------------------------
; VOID *
; EFIAPI
; InternalMemSetMem16 (
;   OUT     VOID                      *Buffer,
;   IN      UINTN                     Length,
;   IN      UINT16                    Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemSetMem16)
ASM_PFX(InternalMemSetMem16):
    push    rdi
    mov     rax, r8
    movq    mm0, rax
    mov     r8, rcx
    mov     rdi, r8
    mov     rcx, rdx
    and     edx, 3
    shr     rcx, 2
    jz      @SetWords
    pshufw  mm0, mm0, 0
.0:
    movntq  [rdi], mm0
    add     rdi, 8
    loop    .0
    mfence
@SetWords:
    mov     ecx, edx
    rep     stosw
    mov     rax, r8
    pop     rdi
    ret


;------------------------------------------------------------------------------
;  VOID *
;  InternalMemSetMem32 (
;    IN VOID   *Buffer,
;    IN UINTN  Count,
;    IN UINT32 Value
;    )
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemSetMem32)
ASM_PFX(InternalMemSetMem32):
    movq    mm0, r8
    mov     rax, rcx                    ; rax <- Buffer
    xchg    rcx, rdx                    ; rcx <- Count  rdx <- Buffer
    shr     rcx, 1                      ; rcx <- # of qwords to set
    jz      @SetDwords
    pshufw  mm0, mm0, 44h
.0:
    movntq  [rdx], mm0
    lea     rdx, [rdx + 8]              ; use "lea" to avoid flag changes
    loop    .0
    mfence
@SetDwords:
    jnc     .1
    movd    [rdx], mm0
.1:
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
    movq    mm0, r8
    mov     rax, rcx                    ; rax <- Buffer
    xchg    rcx, rdx                    ; rcx <- Count
.0:
    movntq  [rdx], mm0
    add     rdx, 8
    loop    .0
    mfence
    ret


;------------------------------------------------------------------------------
;  VOID *
;  InternalMemZeroMem (
;    IN VOID   *Buffer,
;    IN UINTN  Count
;    );
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemZeroMem)
ASM_PFX(InternalMemZeroMem):
    push    rdi
    mov     rdi, rcx
    mov     rcx, rdx
    mov     r8, rdi
    and     edx, 7
    shr     rcx, 3
    jz      @ZeroBytes
    pxor    mm0, mm0
.0:
    movntq  [rdi], mm0
    add     rdi, 8
    loop    .0
    mfence
@ZeroBytes:
    xor     eax, eax
    mov     ecx, edx
    rep     stosb
    mov     rax, r8
    pop     rdi
    ret

