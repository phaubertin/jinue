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

#include <jinue-common/asm/elf.h>
#include <hal/asm/boot.h>
#include <hal/asm/memory.h>
#include <hal/asm/vm.h>
#include <hal/asm/x86.h>

; Stack frame variables and size
#define VAR_ZERO_PAGE       0
#define STACK_FRAME_SIZE    4

; Stack frame allocation
#define allocate_stack_frame()      sub esp, STACK_FRAME_SIZE
#define deallocate_stack_frame()    add esp, STACK_FRAME_SIZE

    bits 32
    
    ; Used to initialize and identify fields that are set by the setup code. The
    ; actual value does not matter.
    %define MUST_BE_SET_BELOW 0
    
    extern kernel_start
    extern kernel_size
    extern proc_start
    extern proc_size

image_start:
    jmp start

    ; Empty string used to represent an empty kernel command line.
empty_string:
    db 0

start:
    ; we are going up
    cld
    
    ; On entry, esi points to the real mode code start/zero-page address
    ;
    ; figure out the size of the kernel image
    mov eax, dword [esi + BOOT_SYSIZE]
    shl eax, 4              ; times 16

    ; align to page boundary
    add eax, PAGE_SIZE - 1
    and eax, ~PAGE_MASK

    ; compute top of kernel image
    add eax, BOOT_SETUP32_ADDR

    ; We put the boot heap immediately after the kernel image and allocate
    ; the boot_info_t structure on that heap.
    ;
    ; Set ebp to the start of the boot_info_t structure. All accesses to that
    ; structure will be relative to ebp.
    mov ebp, eax
    
    ; Set heap pointer taking into account allocation of boot_info_t structure.
    add eax, BOOT_INFO_SIZE
    add eax, 7                      ; align address up...
    and eax, ~7                     ; ... to an eight-byte boundary
    mov dword [ebp + BOOT_INFO_BOOT_HEAP], eax

    ; setup boot stack and heap, then use new stack
    mov eax, ebp
    add eax, BOOT_STACK_HEAP_SIZE           ; add stack and heap size
    add eax, PAGE_SIZE - 1                  ; align address up...
    and eax, ~PAGE_MASK                     ; ... to a page boundary
    mov esp, eax                            ; set stack pointer

    ; Set address for next allocations.
    ;
    ; This setup code allocates some memory right after the kernel image such
    ; as the boot stack and heap set up right above as well as the BIOS memory
    ; map, the initial page tables, etc. See doc/layout.md for the exact layout
    ; of those allocations.
    ;
    ; The edi register is the current top of allocated memory. As a convention,
    ; the functions that are called below that allocate memory take the current
    ; allocation address as argument in edi and return the updated value in edi.
    ; Function that do not allocate memory do not touch edi.
    mov edi, eax

    allocate_stack_frame()

    ; Store real mode code start/zero-page address for later use.
    mov dword [esp + VAR_ZERO_PAGE], esi

    ; Initialize most fields in boot_info_t structure. The rest are initialized
    ; later, except for the location of the boot heap which was already
    ; initialized above.
    call initialize_boot_info

    ; copy data segment and set fields regarding its size and location in the
    ; boot_info_t structure.
    call copy_data_segment

    ; copy BIOS memory map
    mov esi, dword [esp + VAR_ZERO_PAGE]
    call copy_e820_memory_map

    ; copy command line
    mov esi, dword [esp + VAR_ZERO_PAGE]
    call copy_cmdline

    ; Allocate initial page tables and page directory.
    call allocate_page_tables

    ; This is the end of allocations made by this setup code.
    mov dword [ebp + BOOT_INFO_BOOT_END], edi

    ; Initialize initial page tables.
    call initialize_page_tables
    
    ; Initialize initial page directory.
    call initialize_page_directory

    ; Enable paging and protect read-only pages from being written to by the
    ; kernel
    call enable_paging

    ; We won't be using the stack anymore
    deallocate_stack_frame()

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

    ; set address of boot information structure in esi for use by the kernel
    mov esi, ebp
    add esi, BOOT_OFFSET_FROM_1MB   ; adjust to point in kernel alias

    ; null-terminate call stack (useful for debugging)
    xor eax, eax
    push eax
    push eax
    
    ; initialize frame pointer
    xor ebp, ebp

    ; compute kernel entry point address
    mov eax, kernel_start           ; ELF header
    add eax, BOOT_OFFSET_FROM_1MB
    mov eax, [eax + ELF_E_ENTRY]    ; e_entry member

    ; jump to kernel entry point
    jmp eax

    ; ==========================================================================
    ; The End
    ; ==========================================================================

    ;                              *  *  *

    ; --------------------------------------------------------------------------
    ; Function: copy_e820_memory_map
    ; --------------------------------------------------------------------------
    ; Copy BIOS memory map.
    ;
    ; Arguments:
    ;       esi real mode code start/zero-page address
    ;       edi copy destination
    ;       ebp address of the boot_info_t structure
    ;
    ; Returns:
    ;       edi end of memory map
    ;       ecx, esi are caller saved
copy_e820_memory_map:
    ; Set memory map address in boot_info_t structure
    mov dword [ebp + BOOT_INFO_E820_MAP], edi

    ; Source address
    add esi, BOOT_E820_MAP

    ; Compute size to copy
    mov ecx, dword [ebp + BOOT_INFO_E820_ENTRIES]
    lea ecx, [5 * ecx]              ; times 20 (size of one entry), which is 5 ...
    shl ecx, 2                      ; ... times 2^2

    ; Copy memory map
    rep movsb

    ret

    ; --------------------------------------------------------------------------
    ; Function: copy_cmdline
    ; --------------------------------------------------------------------------
    ; Copy kernel command line
    ;
    ; Arguments:
    ;       esi real mode code start/zero-page address
    ;       edi copy destination
    ;       ebp address of the boot_info_t structure
    ;
    ; Returns:
    ;       edi end of kernel command line
    ;       eax, esi are caller saved
copy_cmdline:
    mov dword [ebp + BOOT_INFO_CMDLINE], empty_string

    mov esi, dword [esi + BOOT_CMD_LINE_PTR]
    or esi, esi                     ; if command line pointer is NULL...
    jz .skip                        ; ... skip copy and keep empty string

    ; Following the e820 memory map copy above, edi is already where we
    ; want it to be.
    mov dword [ebp + BOOT_INFO_CMDLINE], edi
.copy:
    lodsb                           ; load next character
    stosb                           ; store character in destination
    or al, al                       ; if character is not terminating NUL...
    jnz .copy                       ; ... continue with next character

.skip:
    ret

    ; --------------------------------------------------------------------------
    ; Function: initialize_boot_info
    ; --------------------------------------------------------------------------
    ; Initialize most fields in the boot_info_t structure, specifically:
    ;
    ;       kernel_start    (ebp + BOOT_INFO_KERNEL_START)
    ;       kernel_size     (ebp + BOOT_INFO_KERNEL_SIZE)
    ;       proc_start      (ebp + BOOT_INFO_PROC_START)
    ;       proc_size       (ebp + BOOT_INFO_PROC_SIZE)
    ;       image_start     (ebp + BOOT_INFO_IMAGE_START)
    ;       image_top       (ebp + BOOT_INFO_IMAGE_TOP)
    ;       ramdisk_start   (ebp + BOOT_INFO_RAMDISK_START)
    ;       ramdisk_size    (ebp + BOOT_INFO_RAMDISK_SIZE)
    ;       setup_signature (ebp + BOOT_INFO_SETUP_SIGNATURE)
    ;       e820_entries    (ebp + BOOT_INFO_E820_ENTRIES)
    ;
    ; Arguments:
    ;       esi real mode code start/zero-page address
    ;       ebp address of the boot_info_t structure
    ;
    ; Returns:
    ;       eax is caller saved
initialize_boot_info:
    ; Values provided by linker.
    mov dword [ebp + BOOT_INFO_KERNEL_START], kernel_start
    mov dword [ebp + BOOT_INFO_KERNEL_SIZE], kernel_size
    mov dword [ebp + BOOT_INFO_PROC_START], proc_start
    mov dword [ebp + BOOT_INFO_PROC_SIZE], proc_size
    mov dword [ebp + BOOT_INFO_IMAGE_START], image_start

    ; set pointer to top of kernel image
    mov dword [ebp + BOOT_INFO_IMAGE_TOP], ebp

    ; Copy initial RAM disk address and size
    mov eax, dword [esi + BOOT_RAMDISK_IMAGE]
    mov dword [ebp + BOOT_INFO_RAMDISK_START], eax
    mov eax, dword [esi + BOOT_RAMDISK_SIZE]
    mov dword [ebp + BOOT_INFO_RAMDISK_SIZE], eax

    ; Copy signature so it can be checked by the kernel
    mov eax, dword [esi + BOOT_SETUP_HEADER]
    mov dword [ebp + BOOT_INFO_SETUP_SIGNATURE], eax

    ; Number of entries in BIOS memory map
    mov al, byte [esi + BOOT_E820_ENTRIES]
    movzx eax, al
    mov dword [ebp + BOOT_INFO_E820_ENTRIES], eax

    ret

    ; --------------------------------------------------------------------------
    ; Function: copy_data_segment
    ; --------------------------------------------------------------------------
    ; Copy the kernel's data segment and set its location and size in the
    ; boot_info_t structure.
    ;
    ; Arguments:
    ;       edi address where data segment will be copied
    ;       ebp address of the boot_info_t structure
    ;
    ; Returns:
    ;       edi address of top of copied data segment (for subsequent allocations)
    ;       eax, ebx, ecx, edx, esi are caller saved
copy_data_segment:
    ; Put current allocation address (eax) in the physical address of data
    ; segment field in the boot_info_t structure and also set it as destination
    ; of data segment copy (edi).
    mov dword [ebp + BOOT_INFO_DATA_PHYSADDR], edi

    ; Check magic numbers at the beginning of the ELF header.
    mov eax, kernel_start
    cmp byte [eax + EI_MAG0], ELF_MAGIC0
    jnz .fail
    cmp byte [eax + EI_MAG1], ELF_MAGIC1
    jnz .fail
    cmp byte [eax + EI_MAG2], ELF_MAGIC2
    jnz .fail
    cmp byte [eax + EI_MAG3], ELF_MAGIC3
    jnz .fail
    cmp byte [eax + EI_CLASS], ELFCLASS32
    jnz .fail
    cmp byte [eax + EI_DATA], ELFDATA2LSB
    jnz .fail

    ; ecx is loop counter when iterating on program headers. Set to number of
    ; program headers.
    mov ecx, dword [eax + ELF_E_PHNUM]

    ; edx is location of current program header. Set to the first program
    ; header.
    mov edx, kernel_start
    add edx, dword [eax + ELF_E_PHOFF]

.loop_phdr:
    ; Only consider loadable segments (segment type PT_LOAD)
    mov eax, dword [edx + ELF_P_TYPE]
    cmp eax, PT_LOAD
    jnz .next_phdr

    ; Only consider read/write segments
    mov eax, dword [edx + ELF_P_FLAGS]
    and eax, PF_W
    jz .next_phdr

    ; We found the segment we were looking for.

    ; Set segment start address in boot_info_t structure
    mov eax, dword [edx + ELF_P_VADDR]      ; Start of data segment
    mov dword [ebp + BOOT_INFO_DATA_START], eax

    ; Set segment size in boot_info_t structure
    mov eax, dword [edx + ELF_P_MEMSZ]      ; Size of data segment in memory
    add eax, PAGE_SIZE - 1                  ; align address up...
    and eax, ~PAGE_MASK                     ; ... to a page boundary
    mov dword [ebp + BOOT_INFO_DATA_SIZE], eax

    ; Set source address of data segment copy
    mov esi, kernel_start                   ; kernel start
    add esi, dword [edx + ELF_P_OFFSET]     ; plus segment offset

    ; Number of bytes to copy
    mov ecx, dword [edx + ELF_P_FILESZ]

    ; Remember number of bytes to zero out for padding (.bss section), which is
    ; size of segment minus size copied. Current value of eax is segment size.
    sub eax, ecx

    ; Copy data segment data
    rep movsb

    ; Set up to zero out remaining of data segment.
    mov ecx, eax    ; size
    mov eax, 0      ; value

    ; Zero out
    rep stosb

    ; Return value: edi is now at the end of the copied data segment
    ret

.next_phdr:
    add edx, ELF_PHDR_SIZE  ; next program header
    dec ecx                 ; decrement loop counter
    jnz .loop_phdr          ; next iteration if loop counter is not zero

.fail:
    ; We couldn't find a writable segment in the kernel's ELF binary, which
    ; means something is seriously wrong. Set data segment start and size fields
    ; to zero and let the kernel deal with this error condition.
    xor eax, eax
    mov dword [ebp + BOOT_INFO_DATA_START], eax
    mov dword [ebp + BOOT_INFO_DATA_SIZE], eax

    ; Return value: edi is unchanged
    ret

    ; --------------------------------------------------------------------------
    ; Function: allocate_page_tables
    ; --------------------------------------------------------------------------
    ; Allocate initial non-PAE page tables and page directory.
    ;
    ; Page tables that need to be allocated:
    ;   - One for the first 2MB of memory, which is where the kernel image is
    ;     located, as well as VGA text video memory.
    ;   - BOOT_PTES_AT_16MB / VM_X86_PAGE_TABLE_PTES for mapping at 0x1000000
    ;     (i.e. at 16MB) where the kernel image will be moved by the kernel.
    ;   - One for kernel image mapping at KLIMIT
    ;
    ; Arguments:
    ;       edi address where tables are allocated
    ;       ebp address of the boot_info_t structure
    ;
    ; Returns:
    ;       edi end of allocations
allocate_page_tables:
    ; Align allocation address to page size before allocating page tables.
    add edi, PAGE_SIZE - 1              ; align address up...
    and edi, ~PAGE_MASK                 ; ... to a page boundary

    ; One page for the first 2MB of memory
    mov dword [ebp + BOOT_INFO_PAGE_TABLE_1MB], edi
    add edi, PAGE_SIZE

    ; Page tables for mapping at 0x1000000 (i.e. at 16MB)
    mov dword [ebp + BOOT_INFO_PAGE_TABLE_16MB], edi
    add edi, PAGE_SIZE * BOOT_PTES_AT_16MB / VM_X86_PAGE_TABLE_PTES

    ; One page table for mapping at KLIMIT
    mov dword [ebp + BOOT_INFO_PAGE_TABLE_KLIMIT], edi
    add edi, PAGE_SIZE

    ; Page directory
    mov dword [ebp + BOOT_INFO_PAGE_DIRECTORY], edi
    add edi, PAGE_SIZE

    ret

    ; --------------------------------------------------------------------------
    ; Function: initialize_page_tables
    ; --------------------------------------------------------------------------
    ; Initialize initial non-PAE page tables.
    ;
    ; Arguments:
    ;       ebp address of the boot_info_t structure
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

    ret

    ; --------------------------------------------------------------------------
    ; Function: initialize_page_directory
    ; --------------------------------------------------------------------------
    ; Initialize initial non-PAE page directory.
    ;
    ; Arguments:
    ;       ebp address of the boot_info_t structure
    ;
    ; Returns:
    ;       eax, ecx, edi are caller saved
initialize_page_directory:
    ; add entry for the first page table
    mov edi, dword [ebp + BOOT_INFO_PAGE_DIRECTORY]
    mov eax, dword [ebp + BOOT_INFO_PAGE_TABLE_1MB]
    or eax, X86_PTE_READ_WRITE | X86_PTE_PRESENT
    mov dword [edi], eax

    ; add entries for page tables for memory at 16MB
    mov eax, dword [ebp + BOOT_INFO_PAGE_TABLE_16MB]
    lea edi, [edi + 4 * (MEMORY_ADDR_16MB >> 22)]
    mov ecx, BOOT_PTES_AT_16MB / VM_X86_PAGE_TABLE_PTES
    call map_linear

    ; add entry for the last page table
    mov edi, dword [ebp + BOOT_INFO_PAGE_DIRECTORY]
    mov eax, dword [ebp + BOOT_INFO_PAGE_TABLE_KLIMIT]
    or eax, X86_PTE_READ_WRITE | X86_PTE_PRESENT
    mov dword [edi + 4 * (KLIMIT >> 22)], eax

    ret

    ; --------------------------------------------------------------------------
    ; Function: enable_paging
    ; --------------------------------------------------------------------------
    ; Enable paging and protect read-only pages from being written to by the
    ; kernel.
    ;
    ; Arguments:
    ;       ebp address of the boot_info_t structure
    ;
    ; Returns:
    ;       eax is caller saved
enable_paging:
    ; set page directory address in CR3
    mov eax, [ebp + BOOT_INFO_PAGE_DIRECTORY]
    mov cr3, eax

    ; enable paging (PG), prevent kernel from writing to read-only pages (WP)
    mov eax, cr0
    or eax, X86_CR0_PG | X86_CR0_WP
    mov cr0, eax

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
    or eax, X86_PTE_READ_WRITE | X86_PTE_PRESENT

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
