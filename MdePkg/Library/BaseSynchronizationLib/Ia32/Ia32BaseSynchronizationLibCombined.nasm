//------------------------------------------------------------------------------
// Combined Ia32 BaseSynchronizationLib NASM Functions
//------------------------------------------------------------------------------
#include "BaseLibInternals.h"

    DEFAULT REL
    SECTION .text


;------------------------------------------------------------------------------
; UINT16
; EFIAPI
; InternalSyncCompareExchange16 (
;   IN      volatile UINT16           *Value,
;   IN      UINT16                    CompareValue,
;   IN      UINT16                    ExchangeValue
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalSyncCompareExchange16)
ASM_PFX(InternalSyncCompareExchange16):
    mov     ecx, [esp + 4]
    mov     ax, [esp + 8]
    mov     dx, [esp + 12]
    lock    cmpxchg [ecx], dx
    ret


;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; InternalSyncCompareExchange32 (
;   IN      volatile UINT32           *Value,
;   IN      UINT32                    CompareValue,
;   IN      UINT32                    ExchangeValue
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalSyncCompareExchange32)
ASM_PFX(InternalSyncCompareExchange32):
    mov     ecx, [esp + 4]
    mov     eax, [esp + 8]
    mov     edx, [esp + 12]
    lock    cmpxchg [ecx], edx
    ret


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; InternalSyncCompareExchange64 (
;   IN      volatile UINT64           *Value,
;   IN      UINT64                    CompareValue,
;   IN      UINT64                    ExchangeValue
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalSyncCompareExchange64)
ASM_PFX(InternalSyncCompareExchange64):
    push    esi
    push    ebx
    mov     esi, [esp + 12]
    mov     eax, [esp + 16]
    mov     edx, [esp + 20]
    mov     ebx, [esp + 24]
    mov     ecx, [esp + 28]
    lock    cmpxchg8b [esi]
    pop     ebx
    pop     esi
    ret


;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; InternalSyncDecrement (
;   IN      volatile UINT32           *Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalSyncDecrement)
ASM_PFX(InternalSyncDecrement):
    mov       ecx, [esp + 4]
    mov       eax, 0FFFFFFFFh
    lock xadd dword [ecx], eax
    dec       eax
    ret

;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; InternalSyncIncrement (
;   IN      volatile UINT32           *Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalSyncIncrement)
ASM_PFX(InternalSyncIncrement):
    mov       ecx, [esp + 4]
    mov       eax, 1
    lock xadd dword [ecx], eax
    inc       eax
    ret

