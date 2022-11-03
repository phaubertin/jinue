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

#ifndef JINUE_KERNEL_I686_VM_PAE_H
#define JINUE_KERNEL_I686_VM_PAE_H

/** This header file contains declarations for the PAE functions defined in
 * hal/vm_pae.c. It is intended to be included by hal/vm.c and hal/vm_pae.c.
 * There should be no reason to include it anywhere else. */

#include <kernel/types.h>

void vm_pae_enable(boot_alloc_t *boot_alloc, const boot_info_t *boot_info);

void vm_pae_create_initial_addr_space(
        addr_space_t    *address_space,
        pte_t           *page_directories,
        boot_alloc_t    *boot_alloc);

addr_space_t *vm_pae_create_addr_space(
        addr_space_t    *addr_space,
        pte_t           *first_page_directory);

void vm_pae_destroy_addr_space(addr_space_t *addr_space);

pte_t *vm_pae_lookup_page_directory(
        addr_space_t    *addr_space,
        void            *addr,
        bool             create_as_needed,
        bool            *reload_cr3);

unsigned int vm_pae_page_table_offset_of(void *addr);

unsigned int vm_pae_page_directory_offset_of(void *addr);

pte_t *vm_pae_get_pte_with_offset(pte_t *pte, unsigned int offset);

void vm_pae_set_pte(pte_t *pte, uint64_t paddr, uint64_t flags);

void vm_pae_set_pte_flags(pte_t *pte, uint64_t flags);

uint64_t vm_pae_get_pte_paddr(const pte_t *pte);

void vm_pae_clear_pte(pte_t *pte);

void vm_pae_copy_pte(pte_t *dest, const pte_t *src);

void vm_pae_create_pdpt_cache(void);

#endif

