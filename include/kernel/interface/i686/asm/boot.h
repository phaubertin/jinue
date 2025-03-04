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

#ifndef JINUE_KERNEL_I686_ASM_BOOT_H
#define JINUE_KERNEL_I686_ASM_BOOT_H

#include <kernel/infrastructure/i686/asm/memory.h>
#include <kernel/machine/asm/machine.h>
#include <kernel/utils/asm/utils.h>

#define BOOT_ADDR_MAP_ENTRIES   0x1e8

#define BOOT_SETUP_SECTS        0x1f1

#define BOOT_SYSIZE             0x1f4

#define BOOT_SIGNATURE          0x1fe

#define BOOT_MAGIC              0xaa55

#define BOOT_SETUP              0x200

#define BOOT_SETUP_HEADER       0x202

#define BOOT_SETUP_MAGIC        0x53726448  /* "HdrS", reversed */

#define BOOT_RAMDISK_IMAGE      0x218

#define BOOT_RAMDISK_SIZE       0x21C

#define BOOT_CMD_LINE_PTR       0x228

#define BOOT_ADDR_MAP           0x2d0

#define BOOT_ADDR_MAP_END       0xd00

#define BOOT_ADDR_MAP_SIZE      (BOOT_ADDR_MAP_END - BOOT_ADDR_MAP)

#define BOOT_SETUP32_ADDR       MEMORY_ADDR_1MB

#define BOOT_SETUP32_SIZE       PAGE_SIZE

#define BOOT_DATA_STRUCT        BOOT_ADDR_MAP_ENTRIES

#define BOOT_STACK_HEAP_SIZE    (4 * PAGE_SIZE)

#define BOOT_OFFSET_FROM_1MB    (KLIMIT - MEMORY_ADDR_1MB)

#define BOOT_OFFSET_FROM_16MB   (KLIMIT - MEMORY_ADDR_16MB)

#define BOOT_SIZE_AT_1MB        (1 * MB)

/* must be a multiple of 4MB (full page tables) */
#define BOOT_SIZE_AT_16MB       (8 * MB)

#define BOOT_PTES_AT_16MB       (BOOT_SIZE_AT_16MB / PAGE_SIZE)

#define BOOT_RAMDISK_LIMIT      0xc0000000

#endif
