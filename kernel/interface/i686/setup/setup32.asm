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

    bits 32

    ; From elf.h
    extern get_kernel_entry_point

    ; From main32.c
    extern main32

    ; This is defined by the linker script
    extern kernel_start
    extern kernel_top

    jmp start

start:
    ; On entry, esi points to the real mode code start/zero-page address

    ; we are going up
    cld

    ; initialize frame pointer
    xor ebp, ebp
    
    ; This setup code allocates some memory right after the kernel image. See
    ; doc/layout.md for the exact layout of those allocations.
    ;
    ; The first thing we allocate are the boot stack and heap. We set ebx to
    ; the start of the heap, which is also the address of the boot information
    ; structure since this is the first thing main32() allocates there.
    mov eax, kernel_top
    mov ebx, eax

    ; We put the boot heap immediately after the kernel image and allocate
    ; the bootinfo_t structure on that heap.
    ;
    ; Set ebx to the start of the bootinfo_t structure. All accesses to that
    ; structure will be relative to ebx.
    mov ebx, eax

    ; setup boot stack and heap, then use new stack
    mov eax, ebx
    add eax, BOOT_STACK_HEAP_SIZE           ; add stack and heap size
    add eax, PAGE_SIZE - 1                  ; align address up...
    and eax, ~PAGE_MASK                     ; ... to a page boundary
    mov esp, eax                            ; set stack pointer

    ; Bigger allocations such as the page tables go after the boot stack and
    ; heap.
    mov edi, eax

    ; null-terminate call stack (useful for debugging)
    xor eax, eax
    push eax
    push eax

    ; Call main32() which does the bulk of the initialization.
    push esi
    push ebx
    push edi

    call main32

    add esp, 12

    ; adjust stack pointer to point in kernel alias
    add esp, BOOT_OFFSET_FROM_1MB

    ; jump to kernel alias
    jmp just_right_here + BOOT_OFFSET_FROM_1MB
just_right_here:

    ; set address of boot information structure in esi for use by the kernel
    mov esi, ebx
    add esi, BOOT_OFFSET_FROM_1MB   ; adjust to point in kernel alias

    ; jump to kernel entry point
    push esi

    call get_kernel_entry_point

    add esp, 4
    jmp eax

    ; ==========================================================================
    ; The End
    ; ==========================================================================

    ;                              *  *  *

    ; -------------------------------------------------------------------------
    ; Function: enable_paging
    ; C prototype: void enable_paging(pte_t *page_directory)
    ; -------------------------------------------------------------------------
    ; Enable paging and protect read-only pages from being written to by the
    ; kernel.
    ; -------------------------------------------------------------------------
    global enable_paging:function (enable_paging.end - enable_paging)
enable_paging:
    ; First argument: page directory address
    mov eax, dword [esp + 4]

    ; set page directory address in CR3
    mov cr3, eax

    ; enable paging (PG), prevent kernel from writing to read-only pages (WP)
    mov eax, cr0
    or eax, X86_CR0_PG | X86_CR0_WP
    mov cr0, eax

    ret
.end:
