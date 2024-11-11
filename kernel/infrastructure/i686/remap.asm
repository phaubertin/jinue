; Copyright (C) 2021 Philippe Aubertin.
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

#include <kernel/infrastructure/i686/pmap/asm/vm.h>
#include <kernel/infrastructure/i686/asm/memory.h>

    bits 32

; ------------------------------------------------------------------------------
; FUNCTION: move_and_remap_kernel
; C PROTOTYPE:
;   void move_and_remap_kernel(
;           addr_t   end_addr,
;           addr_t   pte,
;           uint32_t cr3_value)
; DESCRIPTION:
;   Copy the kernel image and boot stack/heap from its original location at
;   0x100000 (1MB) to 0x1000000 (16MB), then map the copy at KLIMIT where the
;   original is intially mapped. In more detail:
;
;       1. Copy the kernel image and a few allocated pages that immediately
;          follow the kernel image and contain the boot stack and heap from
;          0x100000 (1MB) to 0x1000000 (16MB). (The initial page tables, which
;          are also allocated following the kernel image, are *excluded* from
;          the contiguous range that is copied.
;       2. Modify the initial page tables so the virtual memory range at KLIMIT
;          points to the copy, not the original.
;       3. Reload CR3 to flush the TLB.
;
;   +=======================================+
;   |        initial page directory         |
;   +---------------------------------------+
;   |         initial page tables           |
;   |           (PAE disabled)              |             This range is copied.
;   +---------------------------------------+                             <--+-
;   |         kernel command line           |                                |
;   +---------------------------------------+                                |
;   |       BIOS physical memory map        |                                |
;   +---------------------------------------+                                |
;   |         kernel data segment           |                                |
;   |       (copied from ELF binary)        |                                |
;   +---------------------------------------+                                |
;   |          kernel stack (boot)          |                                |
;   +-----v---------------------------v-----+ <<< Stack pointer              |
;   |                                       |                                |
;   |                . . .                  |                                |
;   |                                       |                                |
;   +-----^---------------------------^-----+                                |
;   |     kernel heap allocations (boot)    | Start of page allocations      |
;   +=======================================+ End of kernel image            |
;   |                                       |                                |
;   |        user space loader (ELF)        |                                |
;   |                                       |                                |
;   +---------------------------------------+                                |
;   |                                       |                                |
;   |           microkernel (ELF)           |                                |
;   |                                       |                                |
;   +---------------------------------------+                                |
;   |           32-bit setup code           |                                |
;   +---------------------------------------+ 0x100000 mapped at KLIMIT   <--+-
;
;   We have to be careful while doing this because it has the side effect of
;   snapshotting the stack and then using the snapshot as the current stack. We
;   don't want to modify the stack between the moment the snapshot is taken and
;   when we start using the copy.
; ------------------------------------------------------------------------------
    global move_and_remap_kernel:function (move_and_remap_kernel.end - move_and_remap_kernel)
move_and_remap_kernel:
    ; Be careful with the calling convention: we must preserve ebx, esi, edi,
    ; ebp and esp
    push esi
    push edi

    ; esp+20:   third argument:  cr3_value
    ; esp+16:   second argument: pte
    ; esp+12:   first argument:  end_addr
    ; esp+8:    esi
    ; esp+4:    edi

    ; --------------------------------------------------------------------------
    ; Step 1: copy
    ; --------------------------------------------------------------------------
    mov esi, MEMORY_ADDR_1MB    ; start address
    mov edi, MEMORY_ADDR_16MB   ; destination address

    mov ecx, [esp+12]           ; first argument: end_addr
    sub ecx, MEMORY_ADDR_1MB    ; compute size to copy
    mov edx, ecx                ; remember copied size for later (edx)
    shr ecx, 2                  ; divide by 4 for number of DWORDs

    rep movsd                   ; copy

    ; --------------------------------------------------------------------------
    ; Step 2: update page tables
    ; --------------------------------------------------------------------------
    mov edi, [esp+16]           ; second argument: address of first PTE
    mov ecx, edx                ; copied size
    shr ecx, PAGE_BITS          ; divide by 4096 for number of pages and PTEs

.loop:
    mov eax,[edi]
    add eax, MEMORY_ADDR_16MB - MEMORY_ADDR_1MB

    ; store eax in PTE pointed to by edi, then add 4 to edi to point to the next
    ; entry
    stosd

    ; decrement ecx, we are done when it reaches 0, otherwise loop
    loop .loop

    ; --------------------------------------------------------------------------
    ; Step 3: reload CR3
    ; --------------------------------------------------------------------------
    mov eax, [esp+20]           ; third argument: cr3_value
    mov cr3, eax                ; set CR3 to flush TLB

    pop edi
    pop esi
    ret
.end:
