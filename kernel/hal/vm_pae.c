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

#include <hal/vm_private.h>
#include <hal/boot.h>
#include <hal/memory.h>
#include <hal/x86.h>
#include <assert.h>
#include <boot.h>
#include <page_alloc.h>
#include <panic.h>
#include <printk.h>
#include <pfalloc.h>
#include <slab.h>
#include <string.h>
#include <util.h>
#include <vmalloc.h>

/** number of address bits that encode the PDPT offset */
#define PDPT_BITS               2

/** number of entries in a Page Directory Pointer Table (PDPT) */
#define PDPT_ENTRIES            (1 << PDPT_BITS)

/** number of entries in a page directory or page table */
#define PAGE_TABLE_ENTRIES      VM_PAE_PAGE_TABLE_PTES


struct pte_t {
    uint64_t entry;
};

struct pdpt_t {
    pte_t   pd[PDPT_ENTRIES];
};

/** slab cache that allocates Page Directory Pointer Tables (PDPTs) */
static slab_cache_t pdpt_cache;

pdpt_t *initial_pdpt;

/** Get the Page Directory Pointer Table (PDPT) index of a virtual address
 *  @param addr virtual address
 * */
static inline unsigned int pdpt_offset_of(void *addr) {
    return (uintptr_t)addr >> (32 - PDPT_BITS);
}

static void clear_pdpt(pdpt_t *pdpt) {
    for(int idx = 0; idx < PDPT_ENTRIES; ++idx) {
        vm_pae_clear_pte(&pdpt->pd[idx]);
    }
}

/**
 * Enable Physical Address Extension (PAE)
 *
 * The 32-bit setup code has set up initial page tables with the following three
 * mappings:
 * 1) First two megabytes of memory are identity mapped.
 * 2) Four megabytes of memory at 0x1000000 (16M) are identity mapped.
 * 3) Two megabytes of memory starting with kernel image mapped at KLIMIT.
 *
 * This function creates new paging structures with the exact same mappings but
 * with PAE format, then enables PAE.
 *
 * Mappings 1) and 3) are both 2MB, which required one page table each. Mapping
 * 2 is 4MB, so it requires two page tables, for a total of four.
 *
 * Mappings 1) and 2) are in the first page directory while mapping 3) is not,
 * so we need two page directories.
 *
 * @param boot_alloc the initialization-time page allocator structure
 *
 * */
void vm_pae_enable(boot_alloc_t *boot_alloc, const boot_info_t *boot_info) {
    pgtable_format_pae = true;
    page_table_entries = VM_PAE_PAGE_TABLE_PTES;

    /* First mapping */
    pte_t *page_table_1mb = boot_page_alloc(boot_alloc);

    vm_initialize_page_table_linear(
            page_table_1mb,
            0,
            VM_FLAG_READ_WRITE,
            PAGE_TABLE_ENTRIES);

    /* Second mapping (two page tables) */
    pte_t *page_table_16mb = boot_page_alloc_n(
            boot_alloc,
            BOOT_PTES_AT_16MB / PAGE_TABLE_ENTRIES);

    vm_initialize_page_table_linear(
            page_table_16mb,
            MEMORY_ADDR_16MB,
            VM_FLAG_READ_WRITE,
            BOOT_PTES_AT_16MB);

    /* Third mapping */
    pte_t *page_table_klimit = boot_page_alloc(boot_alloc);
    uint32_t size_at_16mb = (uint32_t)boot_info->page_table_1mb - MEMORY_ADDR_1MB;
    uint32_t num_entries_at_16mb = size_at_16mb / PAGE_SIZE;

    vm_initialize_page_table_linear(
            page_table_klimit,
            MEMORY_ADDR_16MB,
            VM_FLAG_READ_WRITE,
            num_entries_at_16mb);

    vm_initialize_page_table_linear(
            vm_pae_get_pte_with_offset(page_table_klimit, num_entries_at_16mb),
            MEMORY_ADDR_1MB + size_at_16mb,
            VM_FLAG_READ_WRITE,
            PAGE_TABLE_ENTRIES - num_entries_at_16mb);

    /* Page directory for first two mappings */
    pte_t *low_page_directory = boot_page_alloc(boot_alloc);
    clear_page(low_page_directory);

    vm_pae_set_pte(
            low_page_directory,
            (uintptr_t)page_table_1mb,
            VM_FLAG_READ_WRITE | VM_FLAG_PRESENT);

    vm_initialize_page_table_linear(
            vm_pae_get_pte_with_offset(
                    low_page_directory,
                    vm_pae_page_directory_offset_of((addr_t)MEMORY_ADDR_16MB)),
            (uintptr_t)page_table_16mb,
            VM_FLAG_READ_WRITE,
            BOOT_PTES_AT_16MB / PAGE_TABLE_ENTRIES);

    /* Page directory for third mapping */
    pte_t *page_directory_klimit = boot_page_alloc(boot_alloc);
    clear_page(page_directory_klimit);

    vm_pae_set_pte(
            vm_pae_get_pte_with_offset(
                    page_directory_klimit,
                    vm_pae_page_directory_offset_of((addr_t)KLIMIT)),
            (uintptr_t)page_table_klimit,
            VM_FLAG_READ_WRITE | VM_FLAG_PRESENT);

    /* Initialize PDPT */
    pdpt_t *pdpt = boot_heap_alloc(boot_alloc, pdpt_t, sizeof(pdpt_t));
    clear_pdpt(pdpt);

    vm_pae_set_pte(
            &pdpt->pd[0],
            (uintptr_t)low_page_directory,
            VM_FLAG_PRESENT);

    vm_pae_set_pte(
            &pdpt->pd[pdpt_offset_of((addr_t)KLIMIT)],
            (uintptr_t)page_directory_klimit,
            VM_FLAG_PRESENT);

    enable_pae(PTR_TO_PHYS_ADDR_AT_16MB(pdpt));
}

addr_space_t *vm_pae_create_initial_addr_space(
        pte_t           *page_directories,
        boot_alloc_t    *boot_alloc) {

    /* Allocate initial PDPT. PDPT must be 32-byte aligned. */
    initial_pdpt = boot_heap_alloc(boot_alloc, pdpt_t, 32);
    clear_pdpt(initial_pdpt);

    int offset = pdpt_offset_of((void *)KLIMIT);
    vm_initialize_page_table_linear(
            &initial_pdpt->pd[offset],
            (uintptr_t)page_directories,
            VM_FLAG_PRESENT,
            PDPT_ENTRIES - offset);

    initial_addr_space.top_level.pdpt   = initial_pdpt;
    initial_addr_space.cr3              = VIRT_TO_PHYS_AT_16MB(initial_pdpt);

    return &initial_addr_space;
}

addr_space_t *vm_pae_create_addr_space(addr_space_t *addr_space) {
    /* Create a PDPT for the new address space */
    pdpt_t *pdpt = slab_cache_alloc(&pdpt_cache);

    if(pdpt == NULL) {
        return NULL;
    }

    /* Use the initial address space as a template for the kernel address range
     * (address KLIMIT and above). The page tables for that range are shared by
     * all address spaces. */
    pdpt_t *template_pdpt = initial_addr_space.top_level.pdpt;

    for(int idx = 0; idx < PDPT_ENTRIES; ++idx) {
        pte_t *pdpte = &pdpt->pd[idx];

        if(idx < pdpt_offset_of((addr_t)KLIMIT)) {
            /* This PDPT entry describes an address range entirely under KLIMIT
             * so it is all user space: do not create a page directory at this
             * time. */
             vm_pae_clear_pte(pdpte);
        }
        else {
            /* This page directory describes an address range entirely above
             * KLIMIT: share the template's page directory. */
            vm_pae_copy_pte(pdpte, &template_pdpt->pd[idx]);
        }
    }

    /* Lookup the physical address of the page where the PDPT resides. */
    kern_paddr_t pdpt_page_paddr = vm_lookup_kernel_paddr((addr_t)page_address_of(pdpt));

    /* physical address of PDPT */
    kern_paddr_t pdpt_paddr = pdpt_page_paddr | page_offset_of(pdpt);

    addr_space->top_level.pdpt  = pdpt;
    addr_space->cr3             = pdpt_paddr;

    return addr_space;
}

void vm_pae_destroy_addr_space(addr_space_t *addr_space) {
    unsigned int idx;
    pte_t pdpte;

    pdpt_t *pdpt = addr_space->top_level.pdpt;

    for(idx = 0; idx < PDPT_ENTRIES; ++idx) {
        pdpte.entry = pdpt->pd[idx].entry;

        if(idx < pdpt_offset_of((addr_t)KLIMIT)) {
            /* This page directory describes an address range entirely under
             * KLIMIT so it is all user space: free all page tables in this
             * page directory as well as the page directory itself. */
            if(pdpte.entry & VM_FLAG_PRESENT) {
                vm_destroy_page_directory(
                        vm_pae_get_pte_paddr(&pdpte),
                        0,
                        page_table_entries);
            }
        }
        else {
            /* This page directory describes an address range entirely above
             * KLIMIT: do nothing.
             *
             * The page directory must not be freed because it is shared by all
             * address spaces. */
        }
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
pte_t *vm_pae_lookup_page_directory(
        addr_space_t    *addr_space,
        void            *addr,
        bool             create_as_needed) {

    pdpt_t *pdpt    = addr_space->top_level.pdpt;
    pte_t  *pdpte   = &pdpt->pd[pdpt_offset_of(addr)];
    
    if(vm_pae_get_pte_flags(pdpte) & VM_FLAG_PRESENT) {
        return memory_lookup_page(vm_pae_get_pte_paddr(pdpte));
    }

    if(!create_as_needed) {
        return NULL;
    }

    pte_t *page_directory = page_alloc();

    if(page_directory != NULL) {
        clear_page(page_directory);

        vm_pae_set_pte(
                pdpte,
                vm_lookup_kernel_paddr(page_directory),
                VM_FLAG_PRESENT);
    }

    return page_directory;
}

unsigned int vm_pae_page_table_offset_of(void *addr) {
    return PAGE_TABLE_OFFSET_OF(addr);
}

unsigned int vm_pae_page_directory_offset_of(void *addr) {
    return PAGE_DIRECTORY_OFFSET_OF(addr);
}

pte_t *vm_pae_get_pte_with_offset(pte_t *pte, unsigned int offset) {
    return &pte[offset];
}

/** TODO handle flag bit position > 31 for NX bit support */
void vm_pae_set_pte(pte_t *pte, uint64_t paddr, int flags) {
    pte->entry = paddr | flags;
}

/** TODO handle flag bit position > 31 for NX bit support */
void vm_pae_set_pte_flags(pte_t *pte, int flags) {
    pte->entry = (pte->entry & ~(uint64_t)PAGE_MASK) | flags;
}

int vm_pae_get_pte_flags(const pte_t *pte) {
    return pte->entry & PAGE_MASK;
}

/** TODO mask NX bit as well, maximum 52 bits supported */
uint64_t vm_pae_get_pte_paddr(const pte_t *pte) {
    return (pte->entry & ~(uint64_t)PAGE_MASK);
}

void vm_pae_clear_pte(pte_t *pte) {
    pte->entry = 0;
}

void vm_pae_copy_pte(pte_t *dest, const pte_t *src) {
    dest->entry = src->entry;
}

void vm_pae_create_pdpt_cache(void) {
    slab_cache_init(
            &pdpt_cache,
            "vm_pae_pdpt_cache",
            sizeof(pdpt_t),
            sizeof(pdpt_t),
            NULL,
            NULL,
            SLAB_DEFAULTS);
}
