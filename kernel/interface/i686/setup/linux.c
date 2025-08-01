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

#include <kernel/domain/services/asm/cmdline.h>
#include <kernel/interface/i686/asm/boot.h>
#include <kernel/interface/i686/setup/linux.h>
#include <kernel/interface/i686/types.h>

#define HEADER_FIELD(h, t, o) *(t*)&(h)[o]

/**
 * Initialize some fields in the boot information structure from Linux header
 *
 * @param bootinfo boot information structure
 * @param linux_header Linux x86 boot protocol real-mode kernel header
 */
void initialize_from_linux_header(bootinfo_t *bootinfo, const linux_header_t linux_header) {
    bootinfo->ramdisk_start     = HEADER_FIELD(linux_header, uint32_t, BOOT_RAMDISK_IMAGE);
    bootinfo->ramdisk_size      = HEADER_FIELD(linux_header, uint32_t, BOOT_RAMDISK_SIZE);
    bootinfo->setup_signature   = HEADER_FIELD(linux_header, uint32_t, BOOT_SETUP_HEADER);
    bootinfo->addr_map_entries  = HEADER_FIELD(linux_header, uint8_t, BOOT_ADDR_MAP_ENTRIES);
}

/**
 * Copy the command line string
 * 
 * @param alloc_ptr allocation pointer, i.e. where to allocate memory
 * @param linux_header Linux x86 boot protocol real-mode kernel header
 * @param bootinfo boot information structure
 * @return updated allocation pointer
 */
char *copy_cmdline(char *alloc_ptr, bootinfo_t *bootinfo, const linux_header_t linux_header) {
    const char *src = HEADER_FIELD(linux_header, char *, BOOT_CMD_LINE_PTR);

    bootinfo->cmdline = alloc_ptr;

    if (src != NULL) {
        for(int idx = 0; idx < CMDLINE_MAX_PARSE_LENGTH && src[idx] != '\0'; ++idx) {
            *(alloc_ptr++) = src[idx];
        }
    }

    *alloc_ptr = '\0';
    return alloc_ptr + 1;
}

/**
 * Copy the ACPI (aka. e820) address map
 * 
 * @param alloc_ptr allocation pointer, i.e. where to allocate memory
 * @param linux_header Linux x86 boot protocol real-mode kernel header
 * @param bootinfo boot information structure
 * @return updated allocation pointer
 */
char *copy_acpi_address_map(char *alloc_ptr, bootinfo_t *bootinfo, const linux_header_t linux_header) {
    const char *src = &linux_header[BOOT_ADDR_MAP];
    size_t size     = 20 * HEADER_FIELD(linux_header, uint8_t, BOOT_ADDR_MAP_ENTRIES);

    bootinfo->acpi_addr_map = (acpi_addr_range_t *)alloc_ptr;

    for(int idx = 0; idx < size; ++idx) {
        *(alloc_ptr++) = src[idx];
    }

    return alloc_ptr;
}
