/*
 * Copyright (C) 2024 Philippe Aubertin.
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

#ifndef JINUE_KERNEL_I686_EXPORTS_H
#define JINUE_KERNEL_I686_EXPORTS_H

#include <kernel/typedeps.h>

/* This file contains the machine-specific definitions that need to be visible
 * outside the machine-specific parts of the code. */

/** Physical memory address for use by the kernel */
typedef uint32_t kern_paddr_t;

/** Physical memory address for use by user space */
typedef uint64_t user_paddr_t;

#define PRIxKPADDR PRIx32

#define PRIxUPADDR PRIx64

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
    addr_t saved_stack_pointer;
} machine_thread_t;

typedef struct {
    uint32_t     cr3;
    union {
        pte_t   *pd;   /* non-PAE: page directory */
        pdpt_t  *pdpt; /* PAE: page directory pointer table */
    } top_level;
} addr_space_t;

typedef enum {
    CMDLINE_OPT_PAE_AUTO,
    CMDLINE_OPT_PAE_DISABLE,
    CMDLINE_OPT_PAE_REQUIRE
} cmdline_opt_pae_t;

typedef struct {
    cmdline_opt_pae_t    pae;
    bool                 serial_enable;
    int                  serial_baud_rate;
    int                  serial_ioport;
    bool                 vga_enable;
} machine_cmdline_opts_t;

#endif
