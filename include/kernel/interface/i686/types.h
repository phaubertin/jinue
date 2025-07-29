/*
 * Copyright (C) 2019-2025 Philippe Aubertin.
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

#ifndef JINUE_KERNEL_INTERFACE_I686_TYPES_H
#define JINUE_KERNEL_INTERFACE_I686_TYPES_H

#include <kernel/infrastructure/acpi/types.h>
#include <kernel/infrastructure/i686/types.h>
#include <kernel/types.h>
#include <sys/elf.h>

/* This structure is used by the assembly language setup code to pass
 * information to the kernel. Whenever changes are made to this structure
 * declaration, the constants in the kernel/interface/i686/asm/boot.h must be updated
 * (member offsets and structure size).
 *
 * See also the setup code, i.e. kernel/i686/setup/setup32.asm. */
typedef struct {
    Elf32_Ehdr              *kernel_start;
    size_t                   kernel_size;
    Elf32_Ehdr              *loader_start;
    size_t                   loader_size;
    void                    *image_start;
    void                    *image_top;
    uint32_t                 ramdisk_start;
    size_t                   ramdisk_size;
    const acpi_addr_range_t *acpi_addr_map;
    uint32_t                 addr_map_entries;
    void                    *cmdline;
    void                    *boot_heap;
    void                    *boot_end;
    pte_t                   *page_table_1mb;
    pte_t                   *page_table_16mb;
    pte_t                   *page_table_klimit;
    pte_t                   *page_directory;
    uint32_t                 setup_signature;
    void                    *data_start;
    size_t                   data_size;
    size_t                   data_physaddr;
} bootinfo_t;

typedef struct {
    /* The following four registers are the system call arguments. */
#define msg_arg0 eax
    uint32_t    eax;
#define msg_arg1 ebx
    uint32_t    ebx;
#define msg_arg2 esi
    uint32_t    esi;
#define msg_arg3 edi
    uint32_t    edi;
    uint32_t    edx;
    uint32_t    ecx;
    uint32_t    ds;
    uint32_t    es;
    uint32_t    fs;
    uint32_t    gs;
    uint32_t    errcode;
    uint32_t    ivt;
    uint32_t    ebp;
    uint32_t    eip;
    uint32_t    cs;
    uint32_t    eflags;
    uint32_t    esp;
    uint32_t    ss;
} trapframe_t;

#endif
