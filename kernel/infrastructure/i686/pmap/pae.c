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

#include <kernel/domain/alloc/page_alloc.h>
#include <kernel/domain/alloc/slab.h>
#include <kernel/domain/alloc/vmalloc.h>
#include <kernel/domain/services/panic.h>
#include <kernel/infrastructure/i686/asm/msr.h>
#include <kernel/infrastructure/i686/asm/x86.h>
#include <kernel/infrastructure/i686/isa/instrs.h>
#include <kernel/infrastructure/i686/memory/pages.h>
#include <kernel/infrastructure/i686/pmap/private.h>
#include <kernel/infrastructure/i686/boot_alloc.h>
#include <kernel/infrastructure/i686/cpuinfo.h>
#include <kernel/interface/i686/bootinfo.h>
#include <kernel/utils/pmap.h>
#include <kernel/utils/utils.h>
#include <assert.h>
#include <string.h>

/* This file contains Physical Address Extension (PAE) memory management code.
 * Non-PAE code is located in nopae.c. Virtual memory management code
 * independent of PAE is located in pmap.c. */

/** number of address bits that encode the PDPT offset */
#define PDPT_BITS               2

/** number of entries in a Page Directory Pointer Table (PDPT) */
#define PDPT_ENTRIES            (1 << PDPT_BITS)

/** number of entries in a page directory or page table */
#define PAGE_TABLE_ENTRIES      PAE_PAGE_TABLE_PTES


struct pte_t {
    uint64_t entry;
};

struct pdpt_t {
    pte_t   pd[PDPT_ENTRIES];
};

/** slab cache that allocates Page Directory Pointer Tables (PDPTs) */
static slab_cache_t pdpt_cache;

static pdpt_t *initial_pdpt;

/** Get the Page Directory Pointer Table (PDPT) index of a virtual address
 *  @param addr virtual address
 * */
static inline unsigned int pdpt_offset_of(const void *addr) {
    return (uintptr_t)addr >> (32 - PDPT_BITS);
}

static void clear_pdpt(pdpt_t *pdpt) {
    for(int idx = 0; idx < PDPT_ENTRIES; ++idx) {
        pae_clear_pte(&pdpt->pd[idx]);
    }
}

bool pae_create_addr_space(addr_space_t *addr_space, pte_t *first_page_directory) {
    /* Create a PDPT for the new address space */
    pdpt_t *pdpt = slab_cache_alloc(&pdpt_cache);

    if(pdpt == NULL) {
        return false;
    }

    clear_pdpt(pdpt);

    unsigned int klimit_offset = pdpt_offset_of((void *)JINUE_KLIMIT);

    pae_set_pte(
            &pdpt->pd[klimit_offset],
            machine_lookup_kernel_paddr(first_page_directory),
            X86_PTE_PRESENT);

    for(int idx = klimit_offset + 1; idx < PDPT_ENTRIES; ++idx) {
        pae_copy_pte(&pdpt->pd[idx], &initial_pdpt->pd[idx]);
    }

    /* Lookup the physical address of the page where the PDPT resides. */
    paddr_t pdpt_page_paddr = machine_lookup_kernel_paddr((addr_t)page_address_of(pdpt));

    /* physical address of PDPT */
    paddr_t pdpt_paddr = pdpt_page_paddr | page_offset_of(pdpt);

    addr_space->top_level.pdpt  = pdpt;
    addr_space->cr3             = pdpt_paddr;

    return true;
}

void pae_destroy_addr_space(addr_space_t *addr_space) {
    pdpt_t *pdpt = addr_space->top_level.pdpt;

    for(unsigned int idx = 0; idx < pdpt_offset_of((void *)JINUE_KLIMIT); ++idx) {
        pte_t *pdpte = &pdpt->pd[idx];

        if(pte_is_present(pdpte)) {
            destroy_page_directory(
                    lookup_page_frame_address(pae_get_pte_paddr(pdpte)),
                    entries_per_page_table);
        }
    }

    /* Depending on the value of JINUE_KLIMIT, there are two possible cases
     * with regards to the first kernel page directory, i.e. the first page
     * directory that links kernel page tables:
     *
     * 1. That page directory links *only* kernel page tables, i.e. the page
     *    directory entry for address JINUE_KLIMIT is the first entry of that
     *    page directory. In this case, this page directory is shared by all
     *    address spaces and we must do nothing here. We must not free the
     *    kernel page tables it links to nor the page directory itself.
     *
     * 2. That page directory is split between userspace and the kernel. It
     *    starts with userspace entries, followed by kernel entries, because
     *    the entry for address JINUE_KLIMIT is at some non-zero offset
     *    within this page directory. In this case, we must free the
     *    userspace page tables, but not the kernel page tables, and then free
     *    the page directory. */
    unsigned int klimit_offset = pae_page_directory_offset_of((void *)JINUE_KLIMIT);

    if(klimit_offset > 0) {
        pte_t *pdpte = &pdpt->pd[pdpt_offset_of((void *)JINUE_KLIMIT)];

        assert(pte_is_present(pdpte));

        destroy_page_directory(
                lookup_page_frame_address(pae_get_pte_paddr(pdpte)),
                klimit_offset);
    }

    slab_cache_free(pdpt);
}

/** 
 * Lookup the page directory for a specified address and address space.
 *
 * @param addr_space address space in which the address is looked up.
 * @param addr address to look up
 * @param create_as_need Whether a page table is allocated if it does not exist
 */
pte_t *pae_lookup_page_directory(
        addr_space_t    *addr_space,
        const void      *addr,
        bool             create_as_needed,
        bool            *reload_cr3) {

    /** ASSERTION: addr_space must not be NULL */
    assert(addr_space != NULL);

    /** ASSERTION: reload_cr3 can only be NULL if create_as_needed is false */
    assert(reload_cr3 != NULL || !create_as_needed);

    pdpt_t *pdpt    = addr_space->top_level.pdpt;
    pte_t  *pdpte   = &pdpt->pd[pdpt_offset_of(addr)];
    
    if(pte_is_present(pdpte)) {
        return lookup_page_frame_address(pae_get_pte_paddr(pdpte));
    }

    if(!create_as_needed) {
        return NULL;
    }

    pte_t *page_directory = page_alloc();

    if(page_directory != NULL) {
        clear_page(page_directory);

        pae_set_pte(
                pdpte,
                machine_lookup_kernel_paddr(page_directory),
                X86_PTE_PRESENT);

        /* In 32-bit PAE mode, the CPU stores the four entries of the PDPT
         * in registers. Whenever we modify an entry in the PDPT, we must
         * reload CR3. */
        *reload_cr3 = true;
    }

    return page_directory;
}

/**
 * Get entry offset of specified virtual address within page table
 *
 * @param addr virtual address
 * @return entry offset of address within page table
 */
unsigned int pae_page_table_offset_of(const void *addr) {
    return PAGE_TABLE_OFFSET_OF(addr);
}

/**
 * Get entry offset of specified virtual address within page directory
 *
 * @param addr virtual address
 * @return entry offset of address within page directory
 */
unsigned int pae_page_directory_offset_of(const void *addr) {
    return PAGE_DIRECTORY_OFFSET_OF(addr);
}

/**
 * Get page table entry (PTE) at specified entry offset from specified PTE
 *
 * @param pte base page table entry (e.g. the beginning of a page table)
 * @param offset entry offset
 * @return PTE at specified offset
 */
pte_t *pae_get_pte_with_offset(pte_t *pte, unsigned int offset) {
    return &pte[offset];
}

/**
 * Set page frame address and flags of the specified page table/directory entry
 *
 * The appropriate flags for this function are the architecture-dependent flags,
 * i.e. those defined by the X86_PTE_... constants. See map_page_access_flags()
 * for additional context.
 *
 * @param pte page table or page directory entry
 * @param paddr physical address of page frame
 * @param flags flags
 */
void pae_set_pte(pte_t *pte, uint64_t paddr, uint64_t flags) {
    assert((paddr & ~page_frame_number_mask) == 0);
    pte->entry = paddr | flags;
}

/**
 * Get the physical address set in a page table or page directory entry
 *
 * @param pte page table or page directory entry array
 * @return physical address
 */
uint64_t pae_get_pte_paddr(const pte_t *pte) {
    return (pte->entry & page_frame_number_mask);
}

/**
 * Clear page table or page directory entry
 *
 * Once clear, the entry no longer refers to anything and is not considered
 * present in memory.
 *
 * @param pte page table or page directory entry
 */
void pae_clear_pte(pte_t *pte) {
    pte->entry = 0;
}

/**
 * Copy a page table or page directory entry
 *
 * @param dest destination page table/directory entry
 * @param src source page table/directory entry
 */
void pae_copy_pte(pte_t *dest, const pte_t *src) {
    dest->entry = src->entry;
}

void pae_create_pdpt_cache(void) {
    slab_cache_init(
            &pdpt_cache,
            "pae_pdpt",
            sizeof(pdpt_t),
            sizeof(pdpt_t),
            NULL,
            NULL,
            SLAB_DEFAULTS);
}
