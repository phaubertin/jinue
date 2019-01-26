; Copyright (C) 2019 Philippe Aubertin.
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

#include <hal/asm/boot.h>
#include <hal/asm/vm.h>
#include <hal/asm/x86.h>

    bits 32
    
    ; Used to initialize and identify fields that are set by the setup code. The
    ; actual value does not matter.
    %define MUST_BE_SET_BELOW 0
    
    extern kernel_start
    extern kernel_size
    extern proc_start
    extern proc_size

image_start:
    jmp short header_end

    ; The structure below (i.e. the boot information structure) must match the
    ; boot_info_t struct declaration in include/hal/boot.h.
boot_info_struct:
_kernel_start:      dd kernel_start
_kernel_size:       dd kernel_size
_proc_start:        dd proc_start
_proc_size:         dd proc_size
_image_start:       dd image_start
_image_top:         dd MUST_BE_SET_BELOW
_ramdisk_start:     dd MUST_BE_SET_BELOW
_ramdisk_size:      dd MUST_BE_SET_BELOW
_e820_entries:      dd MUST_BE_SET_BELOW
_e820_map:          dd MUST_BE_SET_BELOW
_cmdline:           dd MUST_BE_SET_BELOW
_boot_heap:         dd MUST_BE_SET_BELOW
_boot_end:          dd MUST_BE_SET_BELOW
_page_table:        dd MUST_BE_SET_BELOW
_page_directory:    dd MUST_BE_SET_BELOW
_setup_signature:   dd MUST_BE_SET_BELOW

    ; Empty string used to represent an empty kernel command line.
empty_string:
    db 0

header_end:
    jmp start

start:
    ; we are going up
    cld
    
    ; On entry, esi points to the real mode code start/zero-page address
    
    ; Copy signature so it can be checked by the kernel
    mov eax, dword [esi + BOOT_SETUP_HEADER]
    mov dword [_setup_signature], eax

    ; Copy initial RAM disk address and size
    mov eax, dword [esi + BOOT_RAMDISK_IMAGE]
    mov dword [_ramdisk_start], eax
    mov eax, dword [esi + BOOT_RAMDISK_SIZE]
    mov dword [_ramdisk_size], eax

    ; figure out the size of the kernel image
    mov eax, dword [esi + BOOT_SYSIZE]
    shl eax, 4              ; times 16
    
    ; align to page boundary
    add eax, PAGE_SIZE - 1
    and eax, ~PAGE_MASK
    
    ; set pointer to top of kernel image
    add eax, BOOT_SETUP32_ADDR
    mov dword [_image_top], eax

    ; --------------------------------------
    ; setup boot-time heap and kernel stack
    ; see doc/layout.txt
    ; --------------------------------------

    ; copy e820 memory map
    mov cl, byte [esi + BOOT_E820_ENTRIES]
    movzx ecx, cl                   ; number of entries
    mov dword [_e820_entries], ecx
    lea ecx, [5 * ecx]              ; times 20 (size of one entry), which is 5 ...
    shl ecx, 2                      ; ... times 2^2
    
    mov edi, eax
    mov dword [_e820_map], edi
    mov ebx, esi                    ; remember current esi in ebx
    add esi, BOOT_E820_MAP
    
    rep movsb

    ; copy command line
    mov dword [_cmdline], empty_string

    mov esi, dword [ebx + BOOT_CMD_LINE_PTR]
    or esi, esi                     ; if command line pointer is NULL...
    jz skip_cmdline_copy            ; ... skip copy and keep empty string

    ; Following the e820 memory map copy above, edi is already where we
    ; want it to be.
    mov dword [_cmdline], edi
cmdline_copy:
    lodsb                           ; load next character
    stosb                           ; store character in destination
    or al, al                       ; if character is not terminating NUL...
    jnz cmdline_copy                ; ... continue with next character

skip_cmdline_copy:
    ; setup boot stack and heap, then use new stack
    mov eax, edi
    add eax, 7                          ; align address up...
    and eax, ~7                         ; ... to an eight-byte boundary
    mov [_boot_heap], eax               ; store heap start address
    
    add eax, BOOT_STACK_HEAP_SIZE       ; add stack and heap size
    add eax, PAGE_SIZE - 1              ; align address up...
    and eax, ~PAGE_MASK                 ; ... to a page boundary
    mov esp, eax                        ; set stack pointer
    
    ; allocate initial page table and page directory
    mov dword [_page_table], eax
    add eax, PAGE_SIZE
    mov dword [_page_directory], eax
    add eax, PAGE_SIZE
    mov dword [_boot_end], eax
    
    ; initialize initial page table for a 1:1 mapping of the first 4MB
    mov eax, 0 | VM_FLAG_READ_WRITE | VM_FLAG_PRESENT   ; start address is 0, flags
    mov edi, dword [_page_table]                        ; write address
    mov ecx, (PAGE_SIZE / 4)                            ; number of entries

init_page_table:
    ; store eax in current page table entry pointed to by edi, then add 4 to edi
    ; to point to the next entry
    stosd
    
    ; update physical address
    add eax, PAGE_SIZE
    
    ; decrement ecx, we are done when it reaches 0, otherwise loop
    loop init_page_table

    ; clear initial page directory
    mov edi, dword [_page_directory]    ; write address
    xor eax, eax                        ; write value: 0
    mov cx, PAGE_SIZE                   ; write PAGE_SIZE bytes
    
    rep stosb
    
    ; add entry for the page table that maps the first 4MB
    mov edi, dword [_page_directory]
    mov eax, dword [_page_table]
    or eax, VM_FLAG_READ_WRITE | VM_FLAG_PRESENT
    mov dword [edi], eax
    
    ; add an alias for the first 4MB of memory at the kernel base address
    mov dword [edi + 4 * (KLIMIT >> 22)], eax
    
    ; set page directory address in CR3
    mov cr3, edi
    
    ; enable paging
    mov eax, cr0
    or eax, X86_CR0_PG
    mov cr0, eax
    
    ; adjust the pointers in the boot information structure so they point in the
    ; kernel alias
    add dword [_kernel_start],      KLIMIT
    add dword [_proc_start],        KLIMIT
    add dword [_image_start],       KLIMIT
    add dword [_image_top],         KLIMIT
    add dword [_e820_map],          KLIMIT
    add dword [_cmdline],           KLIMIT
    add dword [_boot_heap],         KLIMIT
    add dword [_boot_end],          KLIMIT
    add dword [_page_table],        KLIMIT
    add dword [_page_directory],    KLIMIT
    
    ; adjust stack pointer to point in kernel alias
    add esp, KLIMIT

    ; jump to kernel alias to allow the low address alias to be removed
    jmp just_right_here + KLIMIT
just_right_here:
    
    ; remove the low address alias
    mov eax, dword [_page_directory]
    mov dword [eax], 0              ; clear first page table entry
    
    ; reload CR3 to invalidate all TLB entries for the low address alias
    sub eax, KLIMIT     ; we need the physical address
    mov cr3, eax
    
    ; null-terminate call stack (useful for debugging)
    xor eax, eax
    push eax
    push eax
    
    ; initialize frame pointer    
    xor ebp, ebp
    
    ; compute kernel entry point address
    mov esi, kernel_start           ; ELF header
    add esi, KLIMIT
    mov eax, [esi + 24]             ; e_entry member
    
    ; set address of boot information structure in esi for use by the kernel
    mov esi, boot_info_struct
    add esi, KLIMIT                 ; adjust to point in kernel alias
    
    ; jump to kernel entry point
    jmp eax
