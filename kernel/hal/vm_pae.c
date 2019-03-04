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
#include <hal/x86.h>
#include <assert.h>
#include <panic.h>
#include <pfalloc.h>
#include <slab.h>
#include <string.h>
#include <util.h>
#include <vmalloc.h>


/** number of address bits that encode the PDPT offset */
#define PDPT_BITS               2

/** number of entries in a Page Directory Pointer Table (PDPT) */
#define PDPT_ENTRIES            (1 << PDPT_BITS)


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

void vm_pae_boot_init(void) {
    page_table_entries = (size_t)PAGE_TABLE_ENTRIES;
}

/** 
    Lookup and map the page directory for a specified address and address space.
    
    Important note: it is the caller's responsibility to unmap and free the returned
    page directory when it is done with it.
    
    @param addr_space address space in which the address is looked up.
    @param addr address to look up
    @param create_as_need Whether a page table is allocated if it does not exist
*/
pte_t *vm_pae_lookup_page_directory(addr_space_t *addr_space, void *addr, bool create_as_needed) {
    pdpt_t *pdpt    = addr_space->top_level.pdpt;
    pte_t  *pdpte   = &pdpt->pd[pdpt_offset_of(addr)];
    
    if(vm_pae_get_pte_flags(pdpte) & VM_FLAG_PRESENT) {
        /* map page directory */
        pte_t *page_directory   = (pte_t *)vmalloc(global_page_allocator);
        vm_map_kernel((addr_t)page_directory, vm_pae_get_pte_paddr(pdpte), VM_FLAG_READ_WRITE);
        
        return page_directory;
    }
    else {
        if(create_as_needed) {
            /* allocate a new page directory and map it */
            pte_t *page_directory       = (pte_t *)vmalloc(global_page_allocator);
            kern_paddr_t pgdir_paddr    = pfalloc();
        
            vm_map_kernel((addr_t)page_directory, pgdir_paddr, VM_FLAG_READ_WRITE);
            
            /* zero content of page directory */
            memset(page_directory, 0, PAGE_SIZE);
            
            /* link page directory in PDPT */
            vm_pae_set_pte(pdpte, pgdir_paddr, VM_FLAG_PRESENT);
            
            return page_directory;
        }
        else {
            return NULL;
        }
    }
}

unsigned int vm_pae_page_table_offset_of(addr_t addr) {
    return PAGE_TABLE_OFFSET_OF(addr);
}

unsigned int vm_pae_page_directory_offset_of(addr_t addr) {
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

addr_space_t *vm_pae_create_addr_space(addr_space_t *addr_space) {
    unsigned int idx;
    pte_t *pdpte;
    
    /* Create a PDPT for the new address space */
    pdpt_t *pdpt = slab_cache_alloc(&pdpt_cache);

    if(pdpt == NULL) {
        return NULL;
    }

    /* Use the initial address space as a template for the kernel address range
     * (address KLIMIT and above). The page tables for that range are shared by
     * all address spaces. */
    pdpt_t *template_pdpt = initial_addr_space.top_level.pdpt;
    
    for(idx = 0; idx < PDPT_ENTRIES; ++idx) {
        pdpte = &pdpt->pd[idx];
        
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

static void vm_pae_init_low_alias(pdpt_t *pdpt) {
    unsigned int idx;

    /* The 32-bit setup code sets up paging so the first two MB of
     * physical memory is mapped aliased at addresses 0 and KLIMIT.
     * Addresses in the low alias are the same whether paging is
     * enabled or not.
     *
     * The low alias is needed because we need to disable paging while
     * we enable PAE. For this reason, we need to also set up a low
     * alias in the initial address space. We will get rid of this low
     * alias at a later step of initialization once PAE is enabled (see
     * vm_pae_unmap_low_alias()).
     *
     * We only map the first 2MB of physical memory, which is all that
     * a page table gives us in PAE. This is OK because the kernel and
     * early page allocations fit well within this limit and this is all
     * that is needed for early initialization. */
    pte_t *const page_directory = (pte_t *)pfalloc_early();
    pte_t *const page_table     = (pte_t *)pfalloc_early();
    pte_t *const pdpte          = &pdpt->pd[0];

    for(idx = 0; idx < page_table_entries; ++idx) {
        vm_pae_clear_pte(vm_pae_get_pte_with_offset(page_directory, idx));

        vm_pae_set_pte(
                vm_pae_get_pte_with_offset(page_table, idx),
                idx * PAGE_SIZE,
                VM_FLAG_PRESENT);
    }

    vm_pae_set_pte(
            page_directory,
            EARLY_PTR_TO_PHYS_ADDR(page_table),
            VM_FLAG_PRESENT);
    vm_pae_set_pte(
            pdpte,
            EARLY_PTR_TO_PHYS_ADDR(page_directory),
            VM_FLAG_PRESENT);
}

addr_space_t *vm_pae_create_initial_addr_space(boot_heap_t *boot_heap) {
    unsigned int idx;
    
    /* Allocate initial PDPT. PDPT must be 32-byte aligned. */
    initial_pdpt = boot_heap_alloc(boot_heap, pdpt_t, 32);
    
    /* We want the pre-allocated kernel page tables to be contiguous. For this
     * reason, we allocate the page directories first, and then the page tables.
     *
     * This function allocates pages in this order:
     *      +----------------+-------...--------+-------...------+
     *      |    Low alias   |  pre-allocated   |  pre-allocated |
     *      | page directory |      kernel      |     kernel     |
     *      | and page table | page directories |  page tables   |
     *      +----------------+-------...--------+-------...------+
     * */

    for(idx = 0; idx < PDPT_ENTRIES; ++idx) {
        vm_pae_clear_pte(&initial_pdpt->pd[idx]);
    }

    vm_pae_init_low_alias(initial_pdpt);

    const unsigned int last_idx = pdpt_offset_of((addr_t)KERNEL_PREALLOC_LIMIT - 1);

    for(idx = pdpt_offset_of((addr_t)KLIMIT); idx <= last_idx; ++idx) {
        pte_t *const pdpte      = &initial_pdpt->pd[idx];
        pte_t *page_directory   = (pte_t *)pfalloc_early();

        vm_pae_set_pte(
                pdpte,
                EARLY_PTR_TO_PHYS_ADDR(page_directory),
                VM_FLAG_PRESENT);
    }

    for(idx = pdpt_offset_of((addr_t)KLIMIT); idx <= last_idx; ++idx) {
        unsigned int end_index;

        pte_t *const pdpte = &initial_pdpt->pd[idx];
        pte_t *const page_directory = (pte_t *)EARLY_PHYS_TO_VIRT(vm_pae_get_pte_paddr(pdpte));

        if(idx < pdpt_offset_of((addr_t)KERNEL_PREALLOC_LIMIT)) {
            end_index = page_table_entries;
        }
        else {
            end_index = vm_pae_page_directory_offset_of((addr_t)KERNEL_PREALLOC_LIMIT);
        }

        vm_init_page_directory(
                page_directory,
                0,
                end_index,
                idx == pdpt_offset_of((addr_t)KLIMIT));
    }
    
    initial_addr_space.top_level.pdpt   = initial_pdpt;
    initial_addr_space.cr3              = EARLY_VIRT_TO_PHYS(initial_pdpt);
    
    return &initial_addr_space;
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

void vm_pae_unmap_low_alias(addr_space_t *addr_space) {
    /* Enabling PAE requires disabling paging temporarily, which in turn requires
     * an alias of the kernel image region at address 0 to match its physical
     * address. This function gets rid of this alias once PAE is enabled.
     *
     * There is no need for TLB invalidation because the caller reloads CR3 just
     * after calling this function. */
    vm_pae_clear_pte(&addr_space->top_level.pdpt->pd[0]);
}
