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
#include <kernel/interface/i686/setup/alloc.h>
#include <kernel/interface/i686/setup/linux.h>
#include <kernel/interface/i686/types.h>

#define HEADER_FIELD(h, t, o) *(t*)&(h)[o]

/**
 * Initialize some fields in the boot information structure from Linux header
 *
 * @param bootinfo boot information structure
 * @param linux_boot_params Linux x86 boot protocol real-mode kernel header
 */
void initialize_from_linux_boot_params(
    bootinfo_t                  *bootinfo,
    const linux_boot_params_t    linux_boot_params)
{
    bootinfo->ramdisk_start     = HEADER_FIELD(linux_boot_params, uint32_t, BOOT_RAMDISK_IMAGE);
    bootinfo->ramdisk_size      = HEADER_FIELD(linux_boot_params, uint32_t, BOOT_RAMDISK_SIZE);
    bootinfo->setup_signature   = HEADER_FIELD(linux_boot_params, uint32_t, BOOT_SETUP_HEADER);
    bootinfo->addr_map_entries  = HEADER_FIELD(linux_boot_params, uint8_t, BOOT_ADDR_MAP_ENTRIES);
}

/**
 * Copy the command line string
 * 
 * @param bootinfo boot information structure
 * @param linux_boot_params Linux x86 boot protocol real-mode kernel header
 */
void copy_cmdline(bootinfo_t *bootinfo, const linux_boot_params_t linux_boot_params) {
    const char *src = HEADER_FIELD(linux_boot_params, char *, BOOT_CMD_LINE_PTR);

    size_t length = 0;

    if(src != NULL) {
        for(int idx = 0; idx < CMDLINE_MAX_PARSE_LENGTH && src[idx] != '\0'; ++idx) {
            ++length;
        }
    }

    char *dest          = alloc_pages(bootinfo, length + 1);
    bootinfo->cmdline   = dest;

    for(int idx = 0; idx < length; ++idx) {
        dest[idx] = src[idx];
    }

    dest[length] = '\0';
}

/**
 * Copy the ACPI (aka. e820) address map
 * 
 * @param bootinfo boot information structure
 * @param linux_boot_params Linux x86 boot protocol real-mode kernel header
 */
void copy_acpi_address_map(bootinfo_t *bootinfo, const linux_boot_params_t linux_boot_params) {
    size_t size     = 20 * HEADER_FIELD(linux_boot_params, uint8_t, BOOT_ADDR_MAP_ENTRIES);
    const char *src = &linux_boot_params[BOOT_ADDR_MAP];
    char *dest      = alloc_pages(bootinfo, size);

    bootinfo->acpi_addr_map = (acpi_addr_range_t *)dest;

    for(int idx = 0; idx < size; ++idx) {
        dest[idx] = src[idx];
    }
}
