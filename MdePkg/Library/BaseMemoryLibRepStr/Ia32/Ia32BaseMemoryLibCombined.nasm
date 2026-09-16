//------------------------------------------------------------------------------
// Combined Ia32 BaseMemoryLib Assembly Functions for BaseMemoryLibRepStr
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
;    )
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
    cmp     eax, edi
    jae     @CopyBackward               ; Copy backward if overlapped
.0:
    mov     ecx, edx
    and     edx, 3
    shr     ecx, 2
    rep     movsd                       ; Copy as many Dwords as possible
    jmp     @CopyBytes
@CopyBackward:
    mov     esi, eax                    ; esi <- End of Source
    lea     edi, [edi + edx - 1]        ; edi <- End of Destination
    std
@CopyBytes:
    mov     ecx, edx
    rep     movsb                       ; Copy bytes backward
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
    push    edi
    mov     edi, [esp + 8]             ; edi <- Buffer
    mov     ecx, [esp + 12]            ; ecx <- Length
    mov     edx, ecx                   ; edx <- ecx
    shr     ecx, 2                     ; ecx <- number of dwords
    and     edx, 3                     ; edx <- number of trailing bytes
    xor     eax, eax                   ; eax <- 0, also set ZF
    repe    scasd
    jnz     @ReturnFalse               ; ZF=0 means non-zero element found
    mov     ecx, edx
    repe    scasb
    jnz     @ReturnFalse
    pop     edi
    mov     eax, 1                     ; return TRUE
    ret
@ReturnFalse:
    pop     edi
    xor     eax, eax
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
    jz      .0
    mov     eax, ecx
.0:
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
    jz      .0
    mov     eax, ecx
.0:
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
    jz      .2
    mov     eax, ecx
.2:
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
    jz      .0
    mov     eax, ecx
.0:
    pop     edi
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
    push    edi
    mov     eax, [esp + 16]
    mov     edi, [esp + 8]
    mov     ecx, [esp + 12]
    rep     stosb
    mov     eax, [esp + 8]
    pop     edi
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
    push    edi
    mov     eax, [esp + 16]
    mov     edi, [esp + 8]
    mov     ecx, [esp + 12]
    rep     stosw
    mov     eax, [esp + 8]
    pop     edi
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
    push    edi
    mov     eax, [esp + 16]
    mov     edi, [esp + 8]
    mov     ecx, [esp + 12]
    rep     stosd
    mov     eax, [esp + 8]
    pop     edi
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
    push    edi
    mov     ecx, [esp + 12]
    mov     eax, [esp + 16]
    mov     edx, [esp + 20]
    mov     edi, [esp + 8]
.0:
    mov     [edi + ecx*8 - 8], eax
    mov     [edi + ecx*8 - 4], edx
    loop    .0
    mov     eax, edi
    pop     edi
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
    push    edi
    xor     eax, eax
    mov     edi, [esp + 8]
    mov     ecx, [esp + 12]
    mov     edx, ecx
    shr     ecx, 2
    and     edx, 3
    push    edi
    rep     stosd
    mov     ecx, edx
    rep     stosb
    pop     eax
    pop     edi
    ret

