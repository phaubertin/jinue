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

#ifndef JINUE_HAL_VM_PRIVATE_H
#define JINUE_HAL_VM_PRIVATE_H

/** This header file contains private definitions shared by hal/vm.c, hal/vm_pae.c
 * and hal/vm_x86.c. There should be no reason to include it anywhere else. */

#include <jinue-common/vm.h>
#include <hal/vm.h>
#include <hal/vm_pae.h>
#include <hal/vm_x86.h>
#include <stdbool.h>
#include <stdint.h>
#include <types.h>

/** bit mask for page table or page directory offset */
#define PAGE_TABLE_MASK (PAGE_TABLE_ENTRIES - 1)

/** page table entry offset of virtual (linear) address */
#define PAGE_TABLE_OFFSET_OF(x)     ( ((uint32_t)(x) / PAGE_SIZE) & PAGE_TABLE_MASK )

/** page directory entry offset of virtual (linear address) */
#define PAGE_DIRECTORY_OFFSET_OF(x) ( ((uint32_t)(x) / (PAGE_SIZE * PAGE_TABLE_ENTRIES)) & PAGE_TABLE_MASK )

/** page is mapped but inaccessible (mprotect() PROT_NONE)
 *
 * This flag can be mixed with X86_PTE_xxx architectural page flags. Bit 11 is
 * documented as "ignored" by architecture manual. */
#define VM_PTE_PROT_NONE        (1<<11)

extern pte_t *kernel_page_tables;

extern size_t page_table_entries;

extern bool pgtable_format_pae;

void vm_initialize_page_table_linear(
        pte_t       *page_table,
        uint64_t     start_paddr,
        uint64_t     flags,
        int          num_entries);

kern_paddr_t vm_clone_page_directory(
        kern_paddr_t         template_paddr,
        unsigned int         start_index);

void vm_destroy_page_directory(void *page_directory, unsigned int last_index);

#endif
