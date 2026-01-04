; Copyright (C) 2019-2026 Philippe Aubertin.
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
;   - Allocating and setting up a boot-time stack and heap;
;   - Copying the BIOS memory map and the kernel command line;
;   - Loading the kernel's data segment; and
;   - Allocating and initializing the kernel page tables, then enabling paging.
;
; This setup code passes information to the kernel using the boot information
; structure (aka. bootinfo). This structure is declared as type bootinfo_t
; in the kernel/interface/i686/types.h header file.
;
; The boot loader loads the kernel image at address 0x100000 (1MB). This setup
; code then allocates all the memory it needs starting at address 0x1000000
; (16MB).Once it completes and passes control to the kernel, physical memory
; usage looks like this (see doc/layout.md for detail):

; +===================================+                         ===
; |      initial page directory       |                          ^
; +-----------------------------------+ bootinfo.cr3             |
; |       initial page tables         |     (if PAE disabled)    |
; +-----------------------------------+                          | setup code
; |       kernel command line         |                          | page
; +-----------------------------------+                          | allocations
; |     BIOS physical memory map      |                          |
; +-----------------------------------+                          |
; |       kernel data segment         |                          |
; |(copied from ELF binary and padded)|                          v
; +-----------------------------------+                         ---
; |        kernel stack (boot)        |                          ^
; +-----v-----------------------v-----+                          |
; |                                   |                          |
; |              . . .                |                          | kernel boot
; |                                   |                          | stack/heap
; +-----^-----------------------^-----+                          |
; |      PDPT (if PAE enabled)        |                          |
; +-----------------------------------+ bootinfo.cr3             |
; |            bootinfo               |     (if PAE enabled)     |
; |                                   |                          v
; +===================================+ 0x1000000 (16MB)        ===
; |                                   |     
; .                                   .
; .             Unused                .
; .                                   .
; |                                   |
; +===================================+                         ===
; |                                   |                          ^
; |      user space loader (ELF)      |                          |
; |                                   |                          |
; +-----------------------------------+                          |
; |                                   |                          | kernel image
; |         microkernel (ELF)         |                          |
; |                                   |                          |
; +-----------------------------------+                          |
; |         32-bit setup code         |                          |
; |         (i.e. this code)          |                          v
; +===================================+ 0x100000 (1MB)          ===
;                                           
; This gets mapped into the kernel address space with the following layout (see
; doc/layout.md for detail):
;
; +===================================+ bootinfo.boot_end       ===
; |      initial page directory       |                          ^
; +-----------------------------------+ bootinfo.page_directory  |
; |       initial page tables         |                          |
; +-----------------------------------+ bootinfo.page_table      | setup code
; |       kernel command line         |                          | page
; +-----------------------------------+ bootinfo.cmdline         | allocations
; |     BIOS physical memory map      |                          |
; +-----------------------------------+ bootinfo.acpi_addr_map   |
; |    kernel data segment (alias)    |                          v
; +-----------------------------------+                         ---
; |        kernel stack (boot)        |                          ^
; +-----v-----------------------v-----+ (stack pointer)          |
; |                                   |                          |
; |              . . .                |                          | kernel boot
; |                                   |                          | stack/heap
; +-----^-----------------------^-----+ bootinfo.boot_heap       |
; |      PDPT (if PAE enabled)        |                          |
; +-----------------------------------+                          |
; |            bootinfo               |                          v
; +===================================+ bootinfo                ===
; |                                   |     = ALLOC_BASE
; .                                   .
; .             Unused                .
; .                                   .
; |                                   |
; +===================================+ bootinfo.image_top      ===
; |                                   |                          ^
; |      user space loader (ELF)      |                          |
; |                                   |                          |
; +-----------------------------------+ bootinfo.loader_start    |
; |                                   |                          | kernel image
; |            microkernel            |                          |
; |      code and data segments       |                          |
; |                                   |                          |
; +-----------------------------------+ bootinfo.kernel_start    |
; |         32-bit setup code         |                          |
; |         (i.e. this code)          |                          v
; +===================================+ bootinfo.image_start    ===
;                                           = KERNEL_BASE
;                                           = JINUE_KLIMIT (0xc0000000 = 3GB)

#include <kernel/domain/services/asm/cmdline.h>
#include <kernel/infrastructure/i686/pmap/asm/pmap.h>
#include <kernel/infrastructure/i686/asm/cpuid.h>
#include <kernel/infrastructure/i686/asm/eflags.h>
#include <kernel/infrastructure/i686/asm/memory.h>
#include <kernel/infrastructure/i686/asm/msr.h>
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

    jmp start

start:
    ; We are going up!
    cld

    ; initialize frame pointer
    xor ebp, ebp
    
    ; This setup code allocates some memory starting at 0x1000000 (16 MB). See
    ; doc/layout.md for the exact layout of those allocations.
    ;
    ; Setup boot stack.
    mov esp, MEMORY_ADDR_16MB
    add esp, BOOT_STACK_HEAP_SIZE

    ; null-terminate call stack (useful for debugging)
    xor eax, eax
    push eax
    push eax

    ; Call main32() which does the bulk of the initialization.
    ;
    ; On entry, esi points to the real mode code start/zero-page address
    push esi

    call main32

    add esp, 4

    ; Set address of boot information structure returned by main32() in esi for
    ; use by the kernel.
    mov esi, eax

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
    ; C prototype: void enable_paging(bool use_pae, uint32_t cr3)
    ; -------------------------------------------------------------------------
    ; Enable paging and protect read-only pages from being written to by the
    ; kernel.
    ; -------------------------------------------------------------------------
    global enable_paging:function (enable_paging.end - enable_paging)
enable_paging:
    ; set page directory or PDPT address in CR3
    mov eax, dword [esp + 8]    ; second argument: cr3
    mov cr3, eax                ; set cr3

    ; check if PAE needs to be enabled
    mov eax, dword [esp + 4]    ; first argument: use_pae
    or al, al
    jz .nopae                   ; skip if use_pae is not set

    ; enable PAE
    mov eax, cr4
    or eax, X86_CR4_PAE
    mov cr4, eax

    ; enable support for NX/XD bit
    mov ecx, MSR_EFER
    rdmsr
    or eax, MSR_FLAG_EFER_NXE
    wrmsr

.nopae:
    ; enable paging (PG), prevent kernel from writing to read-only pages (WP)
    mov eax, cr0
    or eax, X86_CR0_PG | X86_CR0_WP
    mov cr0, eax

    ret
.end:

    ; -------------------------------------------------------------------------
    ; Function: adjust_stack
    ; C prototype: void adjust_stack(void)
    ; -------------------------------------------------------------------------
    ; The stack pointer and various pointers on the stack originally contain 
    ; physical addresses for use before paging is enabled. This function is
    ; called once paging is enabled to add the proper offset so they point to
    ; the kernel virtual address space.
    ; -------------------------------------------------------------------------
    global adjust_stack:function (adjust_stack.end - adjust_stack)
adjust_stack:
    ; Adjust *the caller's* (i.e. main32()'s) return address and saved frame
    ; pointer.
    ; 
    ; When a C function enters, it does the following:
    ;   1) Push the original frame pointer (ebp) on the stack.
    ;   2) Set the value of the stack pointer (after the push) as the new frame
    ;      pointer (in ebp).
    ;   3) Decrease esp by some amount to allocate the function's stack frame.
    ;
    ; This means ebp points to the pushed frame pointer and just above it is
    ; the return address.
    ;
    ;   ebp + 4: return address
    ;   ebp + 0: pushed frame pointer (original ebp)
    add dword [ebp + 4], BOOT_OFFSET_FROM_1MB

    ; If the pushed frame pointer is zero, which means the top level, we do not
    ; want to touch it.
    mov eax, [ebp + 0]
    or eax, eax
    jz .skipebp

    add dword [ebp + 0], BOOT_OFFSET_FROM_16MB

.skipebp:
    ; Adjust *this function's* return address so when we return, the
    ; instruction pointer will be in the kernel address space.
    add dword [esp], BOOT_OFFSET_FROM_1MB

    ; Finally, adjust the current stack pointer and frame pointer.
    add esp, BOOT_OFFSET_FROM_16MB
    add ebp, BOOT_OFFSET_FROM_16MB

    ret
.end:
