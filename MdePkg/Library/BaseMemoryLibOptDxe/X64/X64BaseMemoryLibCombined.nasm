//------------------------------------------------------------------------------
// Combined X64 BaseMemoryLib Assembly Functions for BaseMemoryLibOptDxe
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
;  VOID *
;  EFIAPI
;  InternalMemSetMem (
;    IN VOID   *Buffer,
;    IN UINTN  Count,
;    IN UINT8  Value
;    )
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemSetMem)
ASM_PFX(InternalMemSetMem):
    push    rdi
    push    rbx
    push    rcx       ; push Buffer
    mov     rax, r8   ; rax = Value
    and     rax, 0xff ; rax = lower 8 bits of r8, upper 56 bits are 0
    mov     ah,  al   ; ah  = al
    mov     bx,  ax   ; bx  = ax
    shl     rax, 0x10  ; rax = ax << 16
    mov     ax,  bx   ; ax  = bx
    mov     rbx, rax  ; ebx = eax
    shl     rax, 0x20  ; rax = rax << 32
    or      rax, rbx  ; eax = ebx
    mov     rdi, rcx  ; rdi = Buffer
    mov     rcx, rdx  ; rcx = Count
    shr     rcx, 3    ; rcx = rcx / 8
    cld
    rep     stosq
    mov     rcx, rdx  ; rcx = rdx
    and     rcx, 7    ; rcx = rcx & 7
    rep     stosb
    pop     rax       ; rax = Buffer
    pop     rbx
    pop     rdi
    ret


;------------------------------------------------------------------------------
;  VOID *
;  EFIAPI
;  InternalMemSetMem16 (
;    IN VOID   *Buffer,
;    IN UINTN  Count,
;    IN UINT16 Value
;    )
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemSetMem16)
ASM_PFX(InternalMemSetMem16):
    push    rdi
    push    rcx
    mov     rdi, rcx
    mov     rax, r8
    xchg    rcx, rdx
    rep     stosw
    pop     rax
    pop     rdi
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
    push    rdi
    push    rcx
    mov     rdi, rcx
    mov     rax, r8
    xchg    rcx, rdx
    rep     stosd
    pop     rax
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
    push    rdi
    push    rcx
    mov     rdi, rcx
    mov     rax, r8
    xchg    rcx, rdx
    rep     stosq
    pop     rax
    pop     rdi
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
    push    rcx       ; push Buffer
    xor     rax, rax  ; rax = 0
    mov     rdi, rcx  ; rdi = Buffer
    mov     rcx, rdx  ; rcx = Count
    shr     rcx, 3    ; rcx = rcx / 8
    and     rdx, 7    ; rdx = rdx & 7
    cld
    rep     stosq
    mov     rcx, rdx  ; rcx = rdx
    rep     stosb
    pop     rax       ; rax = Buffer
    pop     rdi
    ret

