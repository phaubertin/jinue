; Copyright (C) 2019-2024 Philippe Aubertin.
; All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
;
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
;
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
;
; 3. Neither the name of the author nor the names of other contributors
;    may be used to endorse or promote products derived from this software
;    without specific prior written permission.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
; ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
; WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
; DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
; DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
; (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
; ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
; SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <kernel/infrastructure/i686/pmap/asm/pmap.h>
#include <kernel/infrastructure/i686/asm/memory.h>
#include <kernel/interface/i686/asm/boot.h>

    bits 32

; ------------------------------------------------------------------------------
; FUNCTION: enable_pae
; C PROTOTYPE: void enable_pae(uint32_t cr3_value)
; ------------------------------------------------------------------------------
    global enable_pae:function (enable_pae.end - enable_pae)
enable_pae:
    mov eax, [esp+ 4]   ; First argument: pdpt

    ; Jump to low-address alias
    jmp .just_here - BOOT_OFFSET_FROM_16MB
.just_here:

    ; Disable paging.
    mov ecx, cr0
    and ecx, ~X86_CR0_PG
    mov cr0, ecx

    ; Load CR3 with address of PDPT.
    mov cr3, eax

    ; Enable PAE.
    mov eax, cr4
    or eax, X86_CR4_PAE
    mov cr4, eax

    ; Re-enable paging (PG), prevent kernel from writing to read-only pages (WP).
    or ecx, X86_CR0_PG | X86_CR0_WP
    mov cr0, ecx

    ; No need to jump back to high alias. The ret instruction will take care
    ; of it because this is where the return address is.
    ret
.end:
