/*
 * Copyright (C) 2025 Philippe Aubertin.
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the author nor the names of other contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <kernel/interface/i686/asm/boot.h>
#include <kernel/interface/i686/setup/elf.h>
#include <kernel/interface/i686/setup/linkdefs.h>
#include <kernel/interface/i686/setup/linux.h>
#include <kernel/interface/i686/setup/pmap.h>
#include <kernel/interface/i686/setup/setup32.h>
#include <kernel/interface/i686/types.h>

/**
 * Initialize some fields in the boot information structure
 *
 * @param heap_ptr pointer to boot heap (for small allocations)
 * @param linux_header Linux x86 boot protocol real-mode kernel header
 * @return boot information structure
 */
static bootinfo_t *create_bootinfo(char *heap_ptr, const linux_header_t linux_header) {
    bootinfo_t *bootinfo = (bootinfo_t *)heap_ptr;
    heap_ptr = (char *)(bootinfo + 1);

    bootinfo->boot_heap     = heap_ptr;
   
    bootinfo->kernel_start  = (Elf32_Ehdr *)&kernel_start;
    bootinfo->kernel_size   = (size_t)&kernel_size;
    bootinfo->loader_start  = (Elf32_Ehdr *)&loader_start;
    bootinfo->loader_size   = (size_t)&loader_size;
    bootinfo->image_start   = &image_start;

    /* The boot information structure is the first thing allocated right after
     * the kernel image. */
    bootinfo->image_top     = bootinfo;

    initialize_from_linux_header(bootinfo, linux_header);

    return bootinfo;
}

/**
 * Adjust pointers in the boot information structure
 * 
 * The pointers originally contain the physical address for use before paging
 * is enabled. This function adds the proper offset so they point to the kernel
 * virtual address space.
 *
 * @param bootinfo boot information structure
 */
static void adjust_bootinfo_pointers(bootinfo_t *bootinfo) {
#define ADD_OFFSET(p) (p) = (void *)((char *)(p) + BOOT_OFFSET_FROM_1MB)
    ADD_OFFSET(bootinfo->kernel_start);
    ADD_OFFSET(bootinfo->loader_start);
    ADD_OFFSET(bootinfo->image_start);
    ADD_OFFSET(bootinfo->image_top);
    ADD_OFFSET(bootinfo->acpi_addr_map);
    ADD_OFFSET(bootinfo->cmdline);
    ADD_OFFSET(bootinfo->boot_heap);
    ADD_OFFSET(bootinfo->boot_end);
#undef ADD_OFFSET
}

/**
 * Orchestrate setup
 * 
 * @param alloc_ptr allocation pointer, i.e. where to allocate memory
 * @param heap_ptr pointer to boot heap (for small allocations)
 * @param linux_header Linux x86 boot protocol real-mode kernel header
 */
void main32(char *alloc_ptr, char *heap_ptr, const linux_header_t linux_header) {
    bootinfo_t *bootinfo = create_bootinfo(heap_ptr, linux_header);

    alloc_ptr = prepare_data_segment(alloc_ptr, bootinfo);

    alloc_ptr = copy_acpi_address_map(alloc_ptr, bootinfo, linux_header);

    alloc_ptr = copy_cmdline(alloc_ptr, bootinfo, linux_header);

    alloc_ptr = allocate_page_tables(alloc_ptr, bootinfo);

    /* This is the end of allocations made by this setup code. */
    bootinfo->boot_end = alloc_ptr;

    initialize_page_tables(bootinfo);

    initialize_page_directory(bootinfo);

    enable_paging(bootinfo->page_directory);

    adjust_bootinfo_pointers(bootinfo);
}
