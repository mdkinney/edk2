//------------------------------------------------------------------------------
// Combined Ia32 BaseMemoryLib Assembly Functions for BaseMemoryLibMmx
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
;  EFIAPI
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
    mov     ecx, edx
    and     edx, 7
    shr     ecx, 3                      ; ecx <- # of Qwords to copy
    jz      @CopyBytes
    push    eax
    push    eax
    movq    [esp], mm0                  ; save mm0
.1:
    movq    mm0, [esi]
    movq    [edi], mm0
    add     esi, 8
    add     edi, 8
    loop    .1
    movq    mm0, [esp]                  ; restore mm0
    pop     ecx                         ; stack cleanup
    pop     ecx                         ; stack cleanup
    jmp     @CopyBytes
@CopyBackward:
    mov     esi, eax                    ; esi <- Last byte in Source
    lea     edi, [edi + edx - 1]        ; edi <- Last byte in Destination
    std
@CopyBytes:
    mov     ecx, edx
    rep     movsb
    cld
    mov     eax, [esp + 12]
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
;    )
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemSetMem)
ASM_PFX(InternalMemSetMem):
    push    edi
    mov     al, [esp + 16]
    mov     ah, al
    shrd    edx, eax, 16
    shld    eax, edx, 16
    mov     ecx, [esp + 12]             ; ecx <- Count
    mov     edi, [esp + 8]              ; edi <- Buffer
    mov     edx, ecx
    and     edx, 7
    shr     ecx, 3                      ; # of Qwords to set
    jz      @SetBytes
    add     esp, -0x10
    movq    [esp], mm0                  ; save mm0
    movq    [esp + 8], mm1              ; save mm1
    movd    mm0, eax
    movd    mm1, eax
    psllq   mm0, 32
    por     mm0, mm1                    ; fill mm0 with 8 Value's
.0:
    movq    [edi], mm0
    add     edi, 8
    loop    .0
    movq    mm0, [esp]                  ; restore mm0
    movq    mm1, [esp + 8]              ; restore mm1
    add     esp, 0x10                    ; stack cleanup
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
;    )
;------------------------------------------------------------------------------
global ASM_PFX(InternalMemSetMem16)
ASM_PFX(InternalMemSetMem16):
    push    edi
    mov     eax, [esp + 16]
    shrd    edx, eax, 16
    shld    eax, edx, 16
    mov     edx, [esp + 12]
    mov     edi, [esp + 8]
    mov     ecx, edx
    and     edx, 3
    shr     ecx, 2
    jz      @SetWords
    movd    mm0, eax
    movd    mm1, eax
    psllq   mm0, 32
    por     mm0, mm1
.0:
    movq    [edi], mm0
    add     edi, 8
    loop    .0
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
    mov     eax, [esp + 4]              ; eax <- Buffer as return value
    mov     ecx, [esp + 8]              ; ecx <- Count
    movd    mm0, dword [esp + 12]             ; mm0 <- Value
    shr     ecx, 1                      ; ecx <- number of qwords to set
    mov     edx, eax                    ; edx <- Buffer
    jz      @SetDwords
    movq    mm1, mm0
    psllq   mm1, 32
    por     mm0, mm1
.0:
    movq    qword [edx], mm0
    lea     edx, [edx + 8]              ; use "lea" to avoid change in flags
    loop    .0
@SetDwords:
    jnc     .1
    movd    dword [edx], mm0
.1:
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
    mov     eax, [esp + 4]
    mov     ecx, [esp + 8]
    movq    mm0, [esp + 12]
    mov     edx, eax
.0:
    movq    [edx], mm0
    add     edx, 8
    loop    .0
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
    mov     edi, [esp + 8]
    mov     ecx, [esp + 12]
    mov     edx, ecx
    shr     ecx, 3
    jz      @ZeroBytes
    pxor    mm0, mm0
.0:
    movq    [edi], mm0
    add     edi, 8
    loop    .0
@ZeroBytes:
    and     edx, 7
    xor     eax, eax
    mov     ecx, edx
    rep     stosb
    mov     eax, [esp + 8]
    pop     edi
    ret

