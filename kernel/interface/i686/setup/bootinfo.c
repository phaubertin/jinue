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
#include <kernel/interface/i686/setup/linkdefs.h>
#include <kernel/interface/i686/types.h>

/**
 * Initialize some fields in the boot information structure
 *
 * @param bootinfo boot information structure
 * @param linux_header Linux x86 boot protocol real-mode kernel header
 */
void initialize_bootinfo(bootinfo_t *bootinfo, const char *linux_header) {
    bootinfo->kernel_start      = (Elf32_Ehdr *)&kernel_start;
    bootinfo->kernel_size       = (size_t)&kernel_size;
    bootinfo->loader_start      = (Elf32_Ehdr *)&loader_start;
    bootinfo->loader_size       = (size_t)&loader_size;
    bootinfo->image_start       = &image_start;

    /* The boot information structure is the first thing allocated right after
     * the kernel image. */
    bootinfo->image_top         = bootinfo;

    bootinfo->ramdisk_start     = *(uint32_t *)(linux_header + BOOT_RAMDISK_IMAGE);
    bootinfo->ramdisk_size      = *(size_t *)(linux_header + BOOT_RAMDISK_SIZE);
    bootinfo->setup_signature   = *(uint32_t *)(linux_header + BOOT_SETUP_HEADER);
    bootinfo->addr_map_entries  = *(uint8_t *)(linux_header + BOOT_ADDR_MAP_ENTRIES);
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
void adjust_bootinfo_pointers(bootinfo_t *bootinfo) {
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
