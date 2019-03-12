/*
 * Copyright (C) 2019 Philippe Aubertin.
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

#ifndef JINUE_HAL_TYPES_H
#define JINUE_HAL_TYPES_H

#include <jinue-common/elf.h>
#include <hal/asm/descriptors.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Virtual memory address (pointer) with pointer arithmetic allowed */
typedef unsigned char *addr_t;

/** Physical memory address for use by the kernel */
typedef uint32_t kern_paddr_t;

/** Physical memory address for use by user space */
typedef uint64_t user_paddr_t;

/** an invalid page frame address used as null value */
#define PFNULL ((kern_paddr_t)-1)

/** incomplete structure declaration for a page table entry
 *
 * There are actually two different definitions of this structure: one that
 * represents 32-bit entries for standard 32-bit paging, and one that represents
 * 64-bit entries for Physical Address Extension (PAE) paging. The right
 * definition to use is chosen at run time (i.e. during boot).
 *
 * Outside of the specific functions that are used to access information in
 * page table entries, functions are allowed to hold and pass around pointers to
 * page table entries, but are not allowed to dereference them. */
struct pte_t;

/** type of a page table entry */
typedef struct pte_t pte_t;

struct pdpt_t;

typedef struct pdpt_t pdpt_t;

typedef struct {
    /* The assembly language thread switching code makes the assumption that
     * saved_stack_pointer is the first member of this structure. */
    addr_t           saved_stack_pointer;
    addr_t           local_storage_addr;
    size_t           local_storage_size;
} thread_context_t;

typedef struct {
    uint32_t     cr3;
    union {
        kern_paddr_t     pd;    /* non-PAE: page directory */
        pdpt_t          *pdpt;  /* PAE: page directory pointer table */
    } top_level;
} addr_space_t;

typedef struct {
    uint64_t addr;
    uint64_t size;
    uint32_t type;
} e820_t;

/* The declaration below must match the structure defined at the start of the
 * 32-bit setup code. See boot_info_struct in boot/setup32.asm. */
typedef struct {
    Elf32_Ehdr      *kernel_start;
    uint32_t         kernel_size;
    Elf32_Ehdr      *proc_start;
    uint32_t         proc_size;
    void            *image_start;
    void            *image_top;
    uint32_t	     ramdisk_start;
    uint32_t	     ramdisk_size;
    uint32_t         e820_entries;
    const e820_t    *e820_map;
    void            *cmdline;
    void            *boot_heap;
    void            *boot_end;
    pte_t           *page_table;
    pte_t           *page_directory;
    uint32_t         setup_signature;
} boot_info_t;

typedef uint64_t seg_descriptor_t;

typedef uint32_t seg_selector_t;

typedef struct {
    uint16_t padding;
    uint16_t limit;
    addr_t         addr;
} pseudo_descriptor_t;

typedef struct {
    /* offset 0 */
    uint16_t prev;
    /* offset 4 */
    addr_t         esp0;
    /* offset 8 */
    uint16_t ss0;
    /* offset 12 */
    addr_t         esp1;
    /* offset 16 */
    uint16_t ss1;
    /* offset 20 */
    addr_t         esp2;
    /* offset 24 */
    uint16_t ss2;
    /* offset 28 */
    uint32_t  cr3;
    /* offset 32 */
    uint32_t  eip;
    /* offset 36 */
    uint32_t  eflags;
    /* offset 40 */
    uint32_t  eax;
    /* offset 44 */
    uint32_t  ecx;
    /* offset 48 */
    uint32_t  edx;
    /* offset 52 */
    uint32_t  ebx;
    /* offset 56 */
    uint32_t  esp;
    /* offset 60 */
    uint32_t  ebp;
    /* offset 64 */
    uint32_t  esi;
    /* offset 68 */
    uint32_t  edi;
    /* offset 72 */
    uint16_t es;
    /* offset 76 */
    uint16_t cs;
    /* offset 80 */
    uint16_t ss;
    /* offset 84 */
    uint16_t ds;
    /* offset 88 */
    uint16_t fs;
    /* offset 92 */
    uint16_t gs;
    /* offset 96 */
    uint16_t ldt;
    /* offset 100 */
    uint16_t debug;
    uint16_t iomap;
} tss_t;

struct cpu_data_t {
    seg_descriptor_t     gdt[GDT_LENGTH];
    /* The assembly-language system call entry point for the SYSCALL instruction
     * (fast_amd_entry in trap.asm) makes assumptions regarding the location of
     * the TSS within this structure. */
    tss_t                tss;
    struct cpu_data_t   *self;
    addr_space_t        *current_addr_space;
};

typedef struct cpu_data_t cpu_data_t;

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

typedef struct {
    uint32_t    edi;
    uint32_t    esi;
    uint32_t    ebx;
    uint32_t    ebp;
    uint32_t    eip;
} kernel_context_t;

#endif
