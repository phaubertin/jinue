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

; ------------------------------------------------------------------------------
; Among other things, this setup code sets up the initial page tables and then
; enables paging. The mappings are set as follow, all read/write:
;   - The first two megabytes of physical memory are identity mapped. This is
;     where the kernel image is loaded by the boot loader. This mapping also
;     contains other data set up by the boot loader as well as the VGA text
;     video memory.
;   - The four megabytes at 0x1000000 (i.e. 16M) are identity mapped. The kernel
;     moves its own image there as part of its initialization.
;   - The second and third megabytes of physical memory (start address 0x100000)
;     is mapped at KLIMIT (0xc0000000). This maps the kernel image to the
;     virtual address expected by the kernel.

#include <hal/asm/boot.h>
#include <hal/asm/memory.h>
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
    align 4
boot_info_struct:
    dd kernel_start         ; kernel_start
    dd kernel_size          ; kernel_size
    dd proc_start           ; proc_start
    dd proc_size            ; proc_size
    dd image_start          ; image_start
    dd MUST_BE_SET_BELOW    ; image_top
    dd MUST_BE_SET_BELOW    ; ramdisk_start
    dd MUST_BE_SET_BELOW    ; ramdisk_size
    dd MUST_BE_SET_BELOW    ; e820_entries
    dd MUST_BE_SET_BELOW    ; e820_map
    dd MUST_BE_SET_BELOW    ; cmdline
    dd MUST_BE_SET_BELOW    ; boot_heap
    dd MUST_BE_SET_BELOW    ; boot_end
    dd MUST_BE_SET_BELOW    ; page_table_1mb
    dd MUST_BE_SET_BELOW    ; page_table_16mb
    dd MUST_BE_SET_BELOW    ; page_table_klimit
    dd MUST_BE_SET_BELOW    ; page_directory
    dd MUST_BE_SET_BELOW    ; setup_signature

    ; Empty string used to represent an empty kernel command line.
empty_string:
    db 0

header_end:
    jmp start

start:
    ; we are going up
    cld
    
    ; Set ebp to the start of the boot_info_t structure. All accesses to that
    ; structure will be relative to ebp.
    mov ebp, boot_info_struct

    ; On entry, esi points to the real mode code start/zero-page address
    
    ; Copy signature so it can be checked by the kernel
    mov eax, dword [esi + BOOT_SETUP_HEADER]
    mov dword [ebp + BOOT_INFO_SETUP_SIGNATURE], eax

    ; Copy initial RAM disk address and size
    mov eax, dword [esi + BOOT_RAMDISK_IMAGE]
    mov dword [ebp + BOOT_INFO_RAMDISK_START], eax
    mov eax, dword [esi + BOOT_RAMDISK_SIZE]
    mov dword [ebp + BOOT_INFO_RAMDISK_SIZE], eax

    ; figure out the size of the kernel image
    mov eax, dword [esi + BOOT_SYSIZE]
    shl eax, 4              ; times 16
    
    ; align to page boundary
    add eax, PAGE_SIZE - 1
    and eax, ~PAGE_MASK
    
    ; set pointer to top of kernel image
    add eax, BOOT_SETUP32_ADDR
    mov dword [ebp + BOOT_INFO_IMAGE_TOP], eax

    ; --------------------------------------
    ; setup boot-time heap and kernel stack
    ; see doc/layout.txt
    ; --------------------------------------

    ; copy e820 memory map
    mov cl, byte [esi + BOOT_E820_ENTRIES]
    movzx ecx, cl                   ; number of entries
    mov dword [ebp + BOOT_INFO_E820_ENTRIES], ecx
    lea ecx, [5 * ecx]              ; times 20 (size of one entry), which is 5 ...
    shl ecx, 2                      ; ... times 2^2
    
    mov edi, eax
    mov dword [ebp + BOOT_INFO_E820_MAP], edi
    mov ebx, esi                    ; remember current esi in ebx
    add esi, BOOT_E820_MAP
    
    rep movsb

    ; copy command line
    mov dword [ebp + BOOT_INFO_CMDLINE], empty_string

    mov esi, dword [ebx + BOOT_CMD_LINE_PTR]
    or esi, esi                     ; if command line pointer is NULL...
    jz skip_cmdline_copy            ; ... skip copy and keep empty string

    ; Following the e820 memory map copy above, edi is already where we
    ; want it to be.
    mov dword [ebp + BOOT_INFO_CMDLINE], edi
cmdline_copy:
    lodsb                           ; load next character
    stosb                           ; store character in destination
    or al, al                       ; if character is not terminating NUL...
    jnz cmdline_copy                ; ... continue with next character

skip_cmdline_copy:
    ; setup boot stack and heap, then use new stack
    mov eax, edi
    add eax, 7                              ; align address up...
    and eax, ~7                             ; ... to an eight-byte boundary
    mov [ebp + BOOT_INFO_BOOT_HEAP], eax    ; store heap start address
    
    add eax, BOOT_STACK_HEAP_SIZE       ; add stack and heap size
    add eax, PAGE_SIZE - 1              ; align address up...
    and eax, ~PAGE_MASK                 ; ... to a page boundary
    mov esp, eax                        ; set stack pointer
    
    ; Page tables need to be allocated:
    ;   - One for the first 2MB of memory
    ;   - BOOT_PTES_AT_16MB / VM_X86_PAGE_TABLE_PTES for mapping at 0x1000000 (16M)
    ;   - One for kernel image mapping at KLIMIT
    mov dword [ebp + BOOT_INFO_PAGE_TABLE_1MB], eax
    add eax, PAGE_SIZE
    mov dword [ebp + BOOT_INFO_PAGE_TABLE_16MB], eax
    add eax, PAGE_SIZE * BOOT_PTES_AT_16MB / VM_X86_PAGE_TABLE_PTES
    mov dword [ebp + BOOT_INFO_PAGE_TABLE_KLIMIT], eax
    add eax, PAGE_SIZE
    mov dword [ebp + BOOT_INFO_PAGE_DIRECTORY], eax
    add eax, PAGE_SIZE
    mov dword [ebp + BOOT_INFO_BOOT_END], eax

    ; Allocate and initialize initial page tables and page directory.
    call initialize_page_tables
    
    ; set page directory address in CR3
    mov eax, [ebp + BOOT_INFO_PAGE_DIRECTORY]
    mov cr3, eax

    ; enable paging (PG), prevent kernel from writing to read-only pages (WP)
    mov eax, cr0
    or eax, X86_CR0_PG | X86_CR0_WP
    mov cr0, eax

    ; adjust the pointers in the boot information structure so they point in the
    ; kernel alias
    add dword [ebp + BOOT_INFO_KERNEL_START],   BOOT_OFFSET_FROM_1MB
    add dword [ebp + BOOT_INFO_PROC_START],     BOOT_OFFSET_FROM_1MB
    add dword [ebp + BOOT_INFO_IMAGE_START],    BOOT_OFFSET_FROM_1MB
    add dword [ebp + BOOT_INFO_IMAGE_TOP],      BOOT_OFFSET_FROM_1MB
    add dword [ebp + BOOT_INFO_E820_MAP],       BOOT_OFFSET_FROM_1MB
    add dword [ebp + BOOT_INFO_CMDLINE],        BOOT_OFFSET_FROM_1MB
    add dword [ebp + BOOT_INFO_BOOT_HEAP],      BOOT_OFFSET_FROM_1MB
    add dword [ebp + BOOT_INFO_BOOT_END],       BOOT_OFFSET_FROM_1MB
    
    ; adjust stack pointer to point in kernel alias
    add esp, BOOT_OFFSET_FROM_1MB

    ; jump to kernel alias
    jmp just_right_here + BOOT_OFFSET_FROM_1MB
just_right_here:

    ; null-terminate call stack (useful for debugging)
    xor eax, eax
    push eax
    push eax
    
    ; compute kernel entry point address
    mov esi, kernel_start           ; ELF header
    add esi, BOOT_OFFSET_FROM_1MB
    mov eax, [esi + 24]             ; e_entry member
    
    ; set address of boot information structure in esi for use by the kernel
    mov esi, ebp
    add esi, BOOT_OFFSET_FROM_1MB   ; adjust to point in kernel alias
    
    ; initialize frame pointer
    xor ebp, ebp

    ; jump to kernel entry point
    jmp eax

    ; -------------------------------------------------
    ; The End
    ; -------------------------------------------------

    ; --------------------------------------------------------------------------
    ; Function: initialize_page_tables
    ; --------------------------------------------------------------------------
    ; Allocate and initialize initial non-PAE page tables and page directory.
    ;
    ; Arguments:
    ;       (none)
    ;
    ; Returns:
    ;       eax, ebx, ecx, edx, esi, edi are caller saved
initialize_page_tables:
    ; Initialize first page table for a 1:1 mapping of the first 2MB.
    mov eax, 0                                      ; start address is 0
    mov edi, dword [ebp + BOOT_INFO_PAGE_TABLE_1MB] ; write address
    call map_2_megabytes

    ; Initialize pages table to map BOOT_SIZE_AT_16MB starting at 0x1000000 (16M)
    ;
    ; Write address (edi) already has the correct value.
    mov eax, MEMORY_ADDR_16MB           ; start address
    mov ecx, BOOT_PTES_AT_16MB          ; number of page table entries
    call map_linear

    ; Initialize last page table to map 2MB starting with the start of the
    ; kernel image at KLIMIT.
    ;
    ; Write address (edi) already has the correct value.
    mov eax, BOOT_SETUP32_ADDR          ; start address
    call map_2_megabytes

    ; clear initial page directory
    mov edi, dword [ebp + BOOT_INFO_PAGE_DIRECTORY] ; write address
    mov ecx, 1024                       ; write 1024 entries (full table)

    rep stosd

    ; add entry for the first page table
    mov edi, dword [ebp + BOOT_INFO_PAGE_DIRECTORY]
    mov eax, dword [ebp + BOOT_INFO_PAGE_TABLE_1MB]
    or eax, VM_FLAG_READ_WRITE | VM_FLAG_PRESENT
    mov dword [edi], eax

    ; add entries for page tables for memory at 16MB
    mov eax, dword [ebp + BOOT_INFO_PAGE_TABLE_16MB]
    lea edi, [edi + 4 * (MEMORY_ADDR_16MB >> 22)]
    mov ecx, BOOT_PTES_AT_16MB / VM_X86_PAGE_TABLE_PTES
    call map_linear

    ; add entry for the last page table
    mov edi, dword [ebp + BOOT_INFO_PAGE_DIRECTORY]
    mov eax, dword [ebp + BOOT_INFO_PAGE_TABLE_KLIMIT]
    or eax, VM_FLAG_READ_WRITE | VM_FLAG_PRESENT
    mov dword [edi + 4 * (KLIMIT >> 22)], eax

    ret

    ; --------------------------------------------------------------------------
    ; Function: map_linear
    ; --------------------------------------------------------------------------
    ; Initialize consecutive non-PAE page table entries to map consecutive
    ; pages of physical memory (i.e. page frames).
    ;
    ; Arguments:
    ;       eax start physical address
    ;       ecx number of entries
    ;       edi start write address, i.e. address of first page table entry
    ;
    ; Returns:
    ;       edi updated write address
    ;       eax, ecx are caller saved
map_linear:
    ; page table entry flags
    or eax, VM_FLAG_READ_WRITE | VM_FLAG_PRESENT

.loop:
    ; store eax in page table entry pointed to by edi, then add 4 to edi to
    ; point to the next entry
    stosd

    ; update physical address
    add eax, PAGE_SIZE

    ; decrement ecx, we are done when it reaches 0, otherwise loop
    loop .loop

    ret

    ; --------------------------------------------------------------------------
    ; Function: map_2_megabytes
    ; --------------------------------------------------------------------------
    ; Initialize a non-PAE page table to map 2MB of memory. With PAE disabled,
    ; 2MB fills only half a page table. This function fills the remaining half
    ; with non-present entries.
    ;
    ; Arguments:
    ;       eax start physical address
    ;       edi start of page table
    ;
    ; Returns:
    ;       edi start of page following page table
    ;       eax, ecx are caller saved
map_2_megabytes:
    mov ecx, 512                        ; number of entries
    call map_linear

    ; Clear remaining entries.
    xor eax, eax                        ; write value: 0
    mov ecx, 512                        ; write 512 entries (remaining half)

    rep stosd
    ret
