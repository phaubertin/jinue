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

#ifndef JINUE_KERNEL_INFRASTRUCTURE_I686_TYPES_H
#define JINUE_KERNEL_INFRASTRUCTURE_I686_TYPES_H

#include <kernel/infrastructure/i686/asm/descriptors.h>
#include <kernel/infrastructure/i686/exports/types.h>
#include <kernel/machine/types.h>
#include <kernel/types.h>
#include <sys/elf.h>

typedef uint64_t seg_descriptor_t;

typedef uint32_t seg_selector_t;

typedef struct {
    uint16_t    padding;
    uint16_t    limit;
    addr_t      addr;
} pseudo_descriptor_t;

typedef struct {
    /* offset 0 */
    uint16_t    prev;
    /* offset 4 */
    addr_t      esp0;
    /* offset 8 */
    uint16_t    ss0;
    /* offset 12 */
    addr_t      esp1;
    /* offset 16 */
    uint16_t    ss1;
    /* offset 20 */
    addr_t      esp2;
    /* offset 24 */
    uint16_t    ss2;
    /* offset 28 */
    uint32_t    cr3;
    /* offset 32 */
    uint32_t    eip;
    /* offset 36 */
    uint32_t    eflags;
    /* offset 40 */
    uint32_t    eax;
    /* offset 44 */
    uint32_t    ecx;
    /* offset 48 */
    uint32_t    edx;
    /* offset 52 */
    uint32_t    ebx;
    /* offset 56 */
    uint32_t    esp;
    /* offset 60 */
    uint32_t    ebp;
    /* offset 64 */
    uint32_t    esi;
    /* offset 68 */
    uint32_t    edi;
    /* offset 72 */
    uint16_t    es;
    /* offset 76 */
    uint16_t    cs;
    /* offset 80 */
    uint16_t    ss;
    /* offset 84 */
    uint16_t    ds;
    /* offset 88 */
    uint16_t    fs;
    /* offset 92 */
    uint16_t    gs;
    /* offset 96 */
    uint16_t    ldt;
    /* offset 100 */
    uint16_t    debug;
    uint16_t    iomap;
} tss_t;

struct percpu_t {
    /* Assembly language code accesses members in this structure. Make sure to
     * update the PERCPU_OFFSET_... definitions when you change its layout. */
    struct percpu_t     *self;
    addr_space_t        *current_addr_space;
    seg_descriptor_t     gdt[GDT_NUM_ENTRIES];
    tss_t                tss;
};

typedef struct percpu_t percpu_t;

#endif
