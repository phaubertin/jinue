/*
 * Copyright (C) 2019-2024 Philippe Aubertin.
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

#ifndef JINUE_KERNEL_I686_ASM_BOOTINFO_H
#define JINUE_KERNEL_I686_ASM_BOOTINFO_H

/* The following definitions must match the offsets of the members of the
 * bootinfo_t struct defined in interface/i686/types.h. They are used by
 * the setup code, which is written in assembly language. */

/** Offset of the kernel_start bootinfo_t member */
#define BOOTINFO_KERNEL_START        0

/** Offset of the kernel_size bootinfo_t member */
#define BOOTINFO_KERNEL_SIZE         4

/** Offset of the loader_start bootinfo_t member */
#define BOOTINFO_LOADER_START        8

/** Offset of the loader_size bootinfo_t member */
#define BOOTINFO_LOADER_SIZE        12

/** Offset of the image_start bootinfo_t member */
#define BOOTINFO_IMAGE_START        16

/** Offset of the image_top bootinfo_t member */
#define BOOTINFO_IMAGE_TOP          20

/** Offset of the ramdisk_start bootinfo_t member */
#define BOOTINFO_RAMDISK_START      24

/** Offset of the ramdisk_size bootinfo_t member */
#define BOOTINFO_RAMDISK_SIZE       28

/** Offset of the acpi_addr_map bootinfo_t member */
#define BOOTINFO_ACPI_ADDR_MAP      32

/** Offset of the addr_map_entries bootinfo_t member */
#define BOOTINFO_ADDR_MAP_ENTRIES   36

/** Offset of the cmdline bootinfo_t member */
#define BOOTINFO_CMDLINE            40

/** Offset of the boot_heap bootinfo_t member */
#define BOOTINFO_BOOT_HEAP          44

/** Offset of the boot_end bootinfo_t member */
#define BOOTINFO_BOOT_END           48

/** Offset of the page_tables bootinfo_t member */
#define BOOTINFO_PAGE_TABLES        52

/** Offset of the page_directory bootinfo_t member */
#define BOOTINFO_PAGE_DIRECTORY     56

/** Offset of the setup_signature bootinfo_t member */
#define BOOTINFO_SETUP_SIGNATURE    60

/** Size of bootinfo_t */
#define BOOTINFO_SIZE               64

#endif
