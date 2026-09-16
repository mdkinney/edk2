//------------------------------------------------------------------------------
// Combined X64 BaseSynchronizationLib NASM Functions
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
    mov     ax, dx
    lock    cmpxchg [rcx], r8w
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
    mov     eax, edx
    lock    cmpxchg [rcx], r8d
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
    mov     rax, rdx
    lock    cmpxchg [rcx], r8
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
    mov       eax, 0FFFFFFFFh
    lock xadd dword [rcx], eax
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
    mov       eax, 1
    lock xadd dword [rcx], eax
    inc       eax
    ret

