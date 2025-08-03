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

#ifndef JINUE_KERNEL_INFRASTRUCTURE_I686_PMAP_PAE_H
#define JINUE_KERNEL_INFRASTRUCTURE_I686_PMAP_PAE_H

/** This header file contains declarations for the PAE functions defined in
 * kernel/infrastructure/i686/pmap/pae.c. It is intended to be included by
 * pmap.c and pae.c. There should be no reason to include it anywhere else. */

#include <kernel/infrastructure/i686/types.h>
#include <kernel/interface/i686/types.h>

void pae_create_initial_addr_space(
        addr_space_t    *address_space,
        pte_t           *page_directories,
        boot_alloc_t    *boot_alloc);

bool pae_create_addr_space(addr_space_t *addr_space, pte_t *first_page_directory);

void pae_destroy_addr_space(addr_space_t *addr_space);

pte_t *pae_lookup_page_directory(
        addr_space_t    *addr_space,
        const void      *addr,
        bool             create_as_needed,
        bool            *reload_cr3);

unsigned int pae_page_table_offset_of(const void *addr);

unsigned int pae_page_directory_offset_of(const void *addr);

pte_t *pae_get_pte_with_offset(pte_t *pte, unsigned int offset);

void pae_set_pte(pte_t *pte, uint64_t paddr, uint64_t flags);

uint64_t pae_get_pte_paddr(const pte_t *pte);

void pae_clear_pte(pte_t *pte);

void pae_copy_pte(pte_t *dest, const pte_t *src);

void pae_create_pdpt_cache(void);

#endif

