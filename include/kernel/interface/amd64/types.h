/*
 * Copyright (C) 2019-2026 Philippe Aubertin.
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

#ifndef JINUE_KERNEL_INTERFACE_AMD64_TYPES_H
#define JINUE_KERNEL_INTERFACE_AMD64_TYPES_H

#include <kernel/infrastructure/acpi/types.h>
#include <kernel/infrastructure/amd64/types.h>
#include <kernel/types.h>
#include <sys/elf.h>

/* This structure is used by the assembly language setup code to pass
 * information to the kernel. When changes are made to this structure,
 * constants in kernel/interface/amd64/asm/bootinfo.h may need to be updated. */
typedef struct {
    void                    *cmdline;
    Elf64_Ehdr              *kernel_start;
    size_t                   kernel_size;
    Elf64_Ehdr              *loader_start;
    size_t                   loader_size;
    void                    *image_start;
    void                    *image_top;
    uint64_t                 ramdisk_start;
    size_t                   ramdisk_size;
    const acpi_addr_range_t *acpi_addr_map;
    uint32_t                 addr_map_entries;
    void                    *boot_heap;
    void                    *boot_end;
    pte_t                   *page_tables;
    pte_t                   *page_directory;
    uint64_t                 cr3;
    uint8_t                  features;
    uint8_t                  cpu_vendor;
    uint32_t                 setup_signature;
} bootinfo_t;

typedef struct {
    void                    *heap_ptr;
    void                    *current_page;
    void                    *page_limit;
} boot_alloc_t;

/* TODO review this (order, additional registers) */
typedef struct {
    /* The following four registers are the system call arguments. */
#define msg_arg0 rax
    uint64_t    rax;
#define msg_arg1 rbx
    uint64_t    rbx;
#define msg_arg2 rsi
    uint64_t    rsi;
#define msg_arg3 rdi
    uint64_t    rdi;
    uint64_t    rdx;
    uint64_t    rcx;
    uint64_t    ds;
    uint64_t    es;
    uint64_t    fs;
    uint64_t    gs;
    uint64_t    errcode;
    uint64_t    ivt;
    uint64_t    rbp;
    uint64_t    rip;
    uint64_t    cs;
    uint64_t    eflags;
    uint64_t    rsp;
    uint64_t    ss;
} trapframe_t;

#endif
