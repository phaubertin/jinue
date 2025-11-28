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
#include <kernel/interface/i686/setup/alloc.h>
#include <kernel/interface/i686/setup/linkdefs.h>
#include <kernel/interface/i686/setup/linux.h>
#include <kernel/interface/i686/setup/pmap.h>
#include <kernel/interface/i686/setup/setup32.h>
#include <kernel/interface/i686/types.h>

/**
 * Initialize some fields in the boot information structure
 *
 * @param linux_boot_params Linux x86 boot protocol real-mode kernel header
 * @return boot information structure
 */
static bootinfo_t *create_bootinfo(const linux_boot_params_t linux_boot_params) {
    bootinfo_t *bootinfo    = (bootinfo_t *)MEMORY_ADDR_16MB;

    bootinfo->boot_heap     = (char *)(bootinfo + 1);
    bootinfo->boot_end      = (char *)MEMORY_ADDR_16MB + BOOT_STACK_HEAP_SIZE;
   
    bootinfo->kernel_start  = (Elf32_Ehdr *)&kernel_start;
    bootinfo->kernel_size   = (size_t)&kernel_size;
    bootinfo->loader_start  = (Elf32_Ehdr *)&loader_start;
    bootinfo->loader_size   = (size_t)&loader_size;
    bootinfo->image_start   = &image_start;
    bootinfo->image_top     = &image_top;

    initialize_from_linux_boot_params(bootinfo, linux_boot_params);

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
static void adjust_bootinfo_pointers(bootinfo_t **bootinfo) {
#define ADD_OFFSET(p) (p) = (void *)((char *)(p) + BOOT_OFFSET_FROM_1MB)
    ADD_OFFSET((*bootinfo)->kernel_start);
    ADD_OFFSET((*bootinfo)->loader_start);
    ADD_OFFSET((*bootinfo)->image_start);
    ADD_OFFSET((*bootinfo)->image_top);
#undef ADD_OFFSET
#define ADD_OFFSET(p) (p) = (void *)((char *)(p) + BOOT_OFFSET_FROM_16MB)
    ADD_OFFSET((*bootinfo)->acpi_addr_map);
    ADD_OFFSET((*bootinfo)->boot_end);
    ADD_OFFSET((*bootinfo)->boot_heap);
    ADD_OFFSET((*bootinfo)->cmdline);
    ADD_OFFSET((*bootinfo)->page_directory);
    ADD_OFFSET((*bootinfo)->page_tables);
    ADD_OFFSET(*bootinfo);
#undef ADD_OFFSET
}

/**
 * Orchestrate setup
 * 
 * @param alloc_ptr allocation pointer, i.e. where to allocate memory
 * @param heap_ptr pointer to boot heap (for small allocations)
 * @param linux_boot_params Linux x86 boot protocol real-mode kernel header
 */
bootinfo_t *main32(const linux_boot_params_t linux_boot_params) {
    bootinfo_t *bootinfo = create_bootinfo(linux_boot_params);

    data_segment_t data_segment;

    prepare_data_segment(&data_segment, bootinfo);

    allocate_page_tables(bootinfo);

    initialize_page_tables(bootinfo, &data_segment);

    prepare_for_paging(bootinfo);

    enable_paging(bootinfo->use_pae, bootinfo->cr3);

    adjust_bootinfo_pointers(&bootinfo);

    adjust_stack();

    cleanup_after_paging(bootinfo);

    /* Reload CR3 to invalidate TLBs so the changes by cleanup_after_paging()
     * take effect. */
    enable_paging(bootinfo->use_pae, bootinfo->cr3);

    return bootinfo;
}
