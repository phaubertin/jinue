; Copyright (C) 2019-2025 Philippe Aubertin.
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

; -----------------------------------------------------------------------------

; This setup code performs a few tasks to support the kernel's initialization,
; which include:
;   - Allocating a boot-time stack and heap;
;   - Copying the BIOS memory map and the kernel command line;
;   - Loading the kernel's data segment; and
;   - Allocate and initialize the initial page tables, then enabling paging.
;
; The boot loader loads the kernel image at address 0x100000 (1MB). This setup
; code allocates all the memory it needs right after the kernel image. Once it
; completes and passes control to the kernel, the memory layout looks like this
; (see doc/layout.md for detail):

; +===================================+ bootinfo.boot_end       ===
; |      initial page directory       |                          ^
; +-----------------------------------+ bootinfo.page_directory  |
; |       initial page tables         |                          |
; |         (PAE disabled)            |                          |
; +-----------------------------------+ bootinfo.page_table      | setup code
; |       kernel command line         |                          | allocations
; +-----------------------------------+ bootinfo.cmdline         |
; |     BIOS physical memory map      |                          |
; +-----------------------------------+ bootinfo.acpi_addr_map   |
; |       kernel data segment         |                          |
; |     (copied from ELF binary)      |                          v
; +-----------------------------------+ bootinfo.data_physaddr ---
; |        kernel stack (boot)        |                          ^
; +-----v-----------------------v-----+ (stack pointer)          |
; |                                   |                          |
; |              . . .                |                          | kernel boot
; |                                   |                          | stack/heap
; +-----^-----------------------^-----+ bootinfo.boot_heap       |
; |            bootinfo              |                           v
; +===================================+ bootinfo.image_top      ===
; |                                   |                          ^
; |      user space loader (ELF)      |                          |
; |                                   |                          |
; +-----------------------------------+ bootinfo.loader_start    |
; |                                   |                          | kernel image
; |         microkernel (ELF)         |                          |
; |                                   |                          |
; +-----------------------------------+ bootinfo.kernel_start    |
; |         32-bit setup code         |                          |
; |         (i.e. this code)          |                          v
; +===================================+ bootinfo.image_start   ===
;                                           0x100000 (1MB)
;
; The boot information structure (bootinfo) is a data structure this setup code
; allocates on the boot heap and uses to pass information to the kernel. It is
; declared in the hal/types.h header file, with matching constant declarations
; in hal/asm/boot.h that specify the offset of each member of the structure.
;
; The initial page tables initialized by this code define three mappings:
;
;  1) The first two megabytes of physical memory are identity mapped (virtual
;     address = physical address). The kernel image is loaded by the bootloader
;     in this region, at address 0x100000 (1MB). This mapping also contains
;     other data set up by the bootloader as well as the VGA text video memory.
;     The kernel image is mapped read only while the rest of the memory is
;     mapped read/write.
;
;  2) A few megabytes of memory (BOOT_SIZE_AT_16MB) starting at 0x1000000 (i.e.
;     16M) are also identity mapped. The kernel moves its own image there as
;     part of its initialization. This region is mapped read/write.
;
;  3) The part of the second megabyte of physical memory (start address 0x100000)
;     that contain the kernel image as well as the following initial allocations
;     up to but *excluding* the initial page tables and page directory is mapped
;     at KERNEL_BASE. This is where the kernel is intended to be loaded and ran.
;     This is a linear mapping with one exception: the kernel's data segment is
;     a copy of the content in the ELF binary, with appropriate zero-padding for
;     uninitialized data (.bss section) and the copy is mapped read/write at the
;     address where the kernel expects to find it. The rest of the kernel image
; is mapped read only and the rest of the region is read/write.
;
;       +=======================================+ 0x100000000 (4GB)
;       |                                       |
;       .                                       .
;       .               unmapped                .
;       .                                       .
;       |                                       |
;  ===  +=======================================+
;   ^   |          initial allocations          |
;   |   |             (read/write)              |
;   |   +---------------------------------------+
;   |   |          rest of kernel image         |
;       |             (read only)               | --------------------------+
;   3)  +---------------------------------------+                           |
;       |          kernel data segment          |                           |
;   |   |   copy from ELF binary + zero padded  | ----------------------+   |
;   |   |             (read/write)              |                       |   |
;   |   +---------------------------------------+                       |   |
;   |   |             kernel code               |                       |   |
;   |   |     this setup code + text segment    | --------------------------+
;   v   |             (read only)               |                       |   |
;  ===  +=======================================+ 0xc0000000            |   |
;       |                                       | (JINUE_KLIMIT)        |   |
;       .                                       .                       |   |
;       .               unmapped                .                       |   |
;       .                                       .                       |   |
;       |                                       |                       |   |
;  ===  +=======================================+ 0x1000000             |   |
;   ^   |                                       |   + BOOT_SIZE_AT_16MB |   |
;       |                                       |                       |   |
;   2)  |              read/write               |                       |   |
;       |                                       |                       |   |
;   v   |                                       |                       |   |
;  ===  +=======================================+ 0x1000000 (16MB)      |   |
;       |                                       |                       |   |
;       .                                       .                       |   |
;       .               unmapped                .                       |   |
;       .                                       .                       |   |
;       |                                       |                       |   |
;  ===  +=======================================+ 0x200000 (2MB)        |   |
;   ^   |                                       |                       |   |
;   |   |              read/write               |                       |   |
;   |   |                                       |                       |   |
;   |   |^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^|                       |   |
;   |   |           initial allocations         | <---------------------+   |
;   |   +---------------------------------------+ bootinfo.image_top        |
;   |   |             kernel image              |                           |
;   |   |             (read only)               | <-------------------------+
;       +---------------------------------------+ 0x100000 (1MB)
;   1)  |                                       |
;       |              read/write               |
;   |   |                                       |
;   |   +---------------------------------------+ 0xc0000
;   |   |           text video memory           |
;   |   +---------------------------------------+ 0xb8000
;   |   |                                       |
;   |   |              read/write               |
;   v   |                                       |
;  ===  +=======================================+ 0

#include <kernel/domain/services/asm/cmdline.h>
#include <kernel/infrastructure/i686/pmap/asm/pmap.h>
#include <kernel/infrastructure/i686/asm/memory.h>
#include <kernel/infrastructure/i686/asm/x86.h>
#include <kernel/interface/i686/asm/boot.h>
#include <kernel/interface/i686/asm/bootinfo.h>
#include <kernel/utils/asm/pmap.h>
#include <sys/asm/elf.h>

; Stack frame variables and size
#define VAR_ZERO_PAGE       0
#define STACK_FRAME_SIZE    4

; Stack frame allocation
#define allocate_stack_frame()      sub esp, STACK_FRAME_SIZE
#define deallocate_stack_frame()    add esp, STACK_FRAME_SIZE

    bits 32

    ; From bootinfo.c
    extern adjust_bootinfo_pointers
    extern initialize_bootinfo

    ; From pmap.c
    extern allocate_page_tables
    extern initialize_page_directory
    extern initialize_page_tables
    
    ; This is defined by the linker script
    extern kernel_start

    jmp start

    ; Empty string used to represent an empty kernel command line.
empty_string:
    db 0

start:
    ; we are going up
    cld

    ; initialize frame pointer
    xor ebp, ebp
    
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
    ; the bootinfo_t structure on that heap.
    ;
    ; Set ebx to the start of the bootinfo_t structure. All accesses to that
    ; structure will be relative to ebx.
    mov ebx, eax
    
    ; Set heap pointer taking into account allocation of bootinfo_t structure.
    add eax, BOOTINFO_SIZE
    add eax, 7                      ; align address up...
    and eax, ~7                     ; ... to an eight-byte boundary
    mov dword [ebx + BOOTINFO_BOOT_HEAP], eax

    ; setup boot stack and heap, then use new stack
    mov eax, ebx
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

    ; Initialize most fields in bootinfo_t structure. The rest are initialized
    ; later, except for the location of the boot heap which was already
    ; initialized above.
    push esi
    push ebx

    call initialize_bootinfo

    add esp, 8

    ; copy data segment and set fields regarding its size and location in the
    ; bootinfo_t structure.
    call prepare_data_segment

    ; copy BIOS memory map
    mov esi, dword [esp + VAR_ZERO_PAGE]
    call copy_acpi_address_map

    ; copy command line
    mov esi, dword [esp + VAR_ZERO_PAGE]
    call copy_cmdline

    ; Allocate initial page tables and page directory.
    push ebx
    push edi

    call allocate_page_tables

    mov edi, eax
    add esp, 8

    ; This is the end of allocations made by this setup code.
    mov dword [ebx + BOOTINFO_BOOT_END], edi

    ; Initialize initial page tables.
    push ebx

    call initialize_page_tables

    add esp, 4
    
    ; Initialize initial page directory.
    push ebx

    call initialize_page_directory

    add esp, 4

    ; Enable paging and protect read-only pages from being written to by the
    ; kernel
    call enable_paging

    ; adjust the pointers in the boot information structure so they point in the
    ; kernel alias
    push ebx
    
    call adjust_bootinfo_pointers
    
    add esp, 4

    ; We won't be using the stack anymore
    deallocate_stack_frame()

    ; adjust stack pointer to point in kernel alias
    add esp, BOOT_OFFSET_FROM_1MB

    ; jump to kernel alias
    jmp just_right_here + BOOT_OFFSET_FROM_1MB
just_right_here:

    ; set address of boot information structure in esi for use by the kernel
    mov esi, ebx
    add esi, BOOT_OFFSET_FROM_1MB   ; adjust to point in kernel alias

    ; null-terminate call stack (useful for debugging)
    xor eax, eax
    push eax
    push eax

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

    ; -------------------------------------------------------------------------
    ; Function: copy_acpi_address_map
    ; -------------------------------------------------------------------------
    ; Copy ACPI address map.
    ;
    ; Arguments:
    ;       esi real mode code start/zero-page address
    ;       edi copy destination
    ;       ebx address of the bootinfo_t structure
    ;
    ; Returns:
    ;       edi end of memory map
    ;       ecx, esi are caller saved
    ; -------------------------------------------------------------------------
copy_acpi_address_map:
    ; Set memory map address in bootinfo_t structure
    mov dword [ebx + BOOTINFO_ACPI_ADDR_MAP], edi

    ; Source address
    add esi, BOOT_ADDR_MAP

    ; Compute size to copy
    mov ecx, dword [ebx + BOOTINFO_ADDR_MAP_ENTRIES]
    lea ecx, [5 * ecx]              ; times 20 (size of one entry), which is 5 ...
    shl ecx, 2                      ; ... times 2^2

    ; Copy memory map
    rep movsb

    ret

    ; -------------------------------------------------------------------------
    ; Function: copy_cmdline
    ; -------------------------------------------------------------------------
    ; Copy kernel command line
    ;
    ; Arguments:
    ;       esi real mode code start/zero-page address
    ;       edi copy destination
    ;       ebx address of the bootinfo_t structure
    ;
    ; Returns:
    ;       edi end of kernel command line
    ;       eax, ecx, esi are caller saved
    ; -------------------------------------------------------------------------
copy_cmdline:
    mov dword [ebx + BOOTINFO_CMDLINE], empty_string

    mov esi, dword [esi + BOOT_CMD_LINE_PTR]
    or esi, esi                     ; if command line pointer is NULL...
    jz .skip                        ; ... skip copy and keep empty string

    mov dword [ebx + BOOTINFO_CMDLINE], edi
    mov ecx, CMDLINE_MAX_PARSE_LENGTH
.copy:
    lodsb                           ; load next character
    stosb                           ; store character in destination

    dec ecx                         ; decrement max length counter
    jz .too_long                    ; check if maximum length was reached

    or al, al                       ; if character is not terminating NUL...
    jnz .copy                       ; ... continue with next character

.skip:
    ret

.too_long:
    mov al, 0                       ; NUL terminate cropped command line
    stosb
    ret

    ; -------------------------------------------------------------------------
    ; Function: prepare_data_segment
    ; -------------------------------------------------------------------------
    ; Find and copy the kernel's data segment, add zero padding for
    ; uninitialized data and set its location and size in the bootinfo_t
    ; structure.
    ;
    ; Arguments:
    ;       edi address where data segment will be copied
    ;       ebx address of the bootinfo_t structure
    ;
    ; Returns:
    ;       edi address of top of copied data segment (for subsequent allocations)
    ;       eax, ecx, edx, esi are caller saved
    ; -------------------------------------------------------------------------
prepare_data_segment:
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

    ; Put destination address (edi) in the physical address of data segment
    ; field in the bootinfo_t structure.
    mov dword [ebx + BOOTINFO_DATA_PHYSADDR], edi

    ; ecx is the loop counter when iterating on program headers. Set to number
    ; of program headers.
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

    ; Set segment start address in bootinfo_t structure
    mov eax, dword [edx + ELF_P_VADDR]      ; Start of data segment
    mov dword [ebx + BOOTINFO_DATA_START], eax

    ; Set segment size in bootinfo_t structure
    mov eax, dword [edx + ELF_P_MEMSZ]      ; Size of data segment in memory
    add eax, PAGE_SIZE - 1                  ; align address up...
    and eax, ~PAGE_MASK                     ; ... to a page boundary
    mov dword [ebx + BOOTINFO_DATA_SIZE], eax

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
    ; means something is seriously wrong. Set all data segment-related fields to
    ; zero and let the kernel deal with this error condition later.
    xor eax, eax
    mov dword [ebx + BOOTINFO_DATA_START], eax
    mov dword [ebx + BOOTINFO_DATA_SIZE], eax
    mov dword [ebx + BOOTINFO_DATA_PHYSADDR], eax

    ; Return value: edi is unchanged
    ret

    ; -------------------------------------------------------------------------
    ; Function: enable_paging
    ; -------------------------------------------------------------------------
    ; Enable paging and protect read-only pages from being written to by the
    ; kernel.
    ;
    ; Arguments:
    ;       ebx address of the bootinfo_t structure
    ;
    ; Returns:
    ;       eax is caller saved
    ; -------------------------------------------------------------------------
enable_paging:
    ; set page directory address in CR3
    mov eax, [ebx + BOOTINFO_PAGE_DIRECTORY]
    mov cr3, eax

    ; enable paging (PG), prevent kernel from writing to read-only pages (WP)
    mov eax, cr0
    or eax, X86_CR0_PG | X86_CR0_WP
    mov cr0, eax

    ret

    ; -------------------------------------------------------------------------
    ; Function: map_linear
    ; -------------------------------------------------------------------------
    ; Initialize consecutive non-PAE page table entries to map consecutive
    ; pages of physical memory (i.e. page frames).
    ;
    ; Arguments:
    ;       eax start physical address and access flags
    ;       ecx number of entries
    ;       edi start write address, i.e. address of first page table entry
    ;
    ; Returns:
    ;       edi updated write address
    ;       eax, ecx are caller saved
    ; -------------------------------------------------------------------------
map_linear:
    ; page table entry flags
    or eax, X86_PTE_PRESENT

.loop:
    ; store eax in page table entry pointed to by edi, then add 4 to edi to
    ; point to the next entry
    stosd

    ; update physical address
    add eax, PAGE_SIZE

    ; decrement ecx, we are done when it reaches 0, otherwise loop
    loop .loop

    ret

    ; -------------------------------------------------------------------------
    ; Function: clear_ptes
    ; -------------------------------------------------------------------------
    ; clear consecutive non-PAE page table entries.
    ;
    ; Arguments:
    ;       ecx number of entries
    ;       edi start write address, i.e. address of first page table entry
    ;
    ; Returns:
    ;       edi updated write address
    ;       eax, ecx are caller saved
    ; -------------------------------------------------------------------------
clear_ptes:
    xor eax, eax                        ; write value: 0
    rep stosd                           ; clear entries

    ret
