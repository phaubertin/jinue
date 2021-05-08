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
#include <boot.h>
#include <page_alloc.h>
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

void vm_pae_boot_init(void) {
    page_table_entries = (size_t)PAGE_TABLE_ENTRIES;
}

/**
 * Initialize consecutive page table entries to map consecutive page frames
 *
 * @param page_table first page table entry
 * @param start_paddr start physical address
 * @param flags page table entry flags
 * @param num_entries number of entries to initialize
 *
 * */
static void initialize_page_table_linear(
        pte_t       *page_table,
        uint64_t     start_paddr,
        int          flags,
        int          num_entries) {

    pte_t *pte      = page_table;
    uint64_t paddr  = start_paddr;

    for(int idx = 0; idx < num_entries; ++idx) {
        vm_pae_set_pte(pte, paddr, flags | VM_FLAG_PRESENT);

        paddr += PAGE_SIZE;
        ++pte;
    }
}

static void clear_pdpt(pdpt_t *pdpt) {
    for(int idx = 0; idx < PDPT_ENTRIES; ++idx) {
        vm_pae_clear_pte(&pdpt->pd[idx]);
    }
}

/**
 * Enable Physical Address Extension (PAE)
 *
 * The 32-bit setup code has set up intial page tables with the following three
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
void vm_pae_enable(boot_alloc_t *boot_alloc) {
    /* First mapping */
    pte_t *page_table1 = (pte_t *)boot_page_alloc_early(boot_alloc);
    initialize_page_table_linear(
            page_table1,
            0,
            VM_FLAG_READ_WRITE,
            PAGE_TABLE_ENTRIES);

    /* Second mapping (two page tables) */
    pte_t *page_table2 = (pte_t *)boot_page_alloc_n_early(
            boot_alloc,
            BOOT_PTES_AT_16MB / PAGE_TABLE_ENTRIES);
    initialize_page_table_linear(
            page_table2,
            MEM_ADDR_16MB,
            VM_FLAG_READ_WRITE,
            BOOT_PTES_AT_16MB);

    /* Third mapping */
    pte_t *page_table3 = (pte_t *)boot_page_alloc_early(boot_alloc);
    initialize_page_table_linear(
            page_table3,
            BOOT_SETUP32_ADDR,
            VM_FLAG_READ_WRITE,
            PAGE_TABLE_ENTRIES);

    /* Page directory for first two mappings */
    pte_t *page_directory12 = (pte_t *)boot_page_alloc_early(boot_alloc);
    clear_page(page_directory12);

    vm_pae_set_pte(
            page_directory12,
            EARLY_PTR_TO_PHYS_ADDR(page_table1),
            VM_FLAG_READ_WRITE | VM_FLAG_PRESENT);

    for(int idx = 0; idx < BOOT_PTES_AT_16MB / PAGE_TABLE_ENTRIES; ++idx) {
        const int offset = idx * PAGE_SIZE;
        vm_pae_set_pte(
                vm_pae_get_pte_with_offset(
                        page_directory12,
                        vm_pae_page_directory_offset_of((addr_t)MEM_ADDR_16MB + offset)),
                EARLY_PTR_TO_PHYS_ADDR(page_table2) + offset,
                VM_FLAG_READ_WRITE | VM_FLAG_PRESENT);
    }

    /* Page directory for third mapping */
    pte_t *page_directory3 = (pte_t *)boot_page_alloc_early(boot_alloc);
    clear_page(page_directory3);

    vm_pae_set_pte(
            vm_pae_get_pte_with_offset(
                    page_directory3,
                    vm_pae_page_directory_offset_of((addr_t)KLIMIT)),
            EARLY_PTR_TO_PHYS_ADDR(page_table3),
            VM_FLAG_READ_WRITE | VM_FLAG_PRESENT);

    /* Initialize PDPT */
    pdpt_t *pdpt = boot_heap_alloc(boot_alloc, pdpt_t, sizeof(pdpt_t));
    clear_pdpt(pdpt);

    vm_pae_set_pte(
            &pdpt->pd[0],
            EARLY_PTR_TO_PHYS_ADDR(page_directory12),
            VM_FLAG_PRESENT);

    vm_pae_set_pte(
            &pdpt->pd[pdpt_offset_of((addr_t)KLIMIT)],
            EARLY_PTR_TO_PHYS_ADDR(page_directory3),
            VM_FLAG_PRESENT);

    enable_pae(EARLY_PTR_TO_PHYS_ADDR(pdpt));
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
        pte_t *page_directory   = (pte_t *)vmalloc();
        vm_map_kernel((addr_t)page_directory, vm_pae_get_pte_paddr(pdpte), VM_FLAG_READ_WRITE);
        
        return page_directory;
    }
    else {
        if(create_as_needed) {
            /* allocate a new page directory and map it */
            pte_t *page_directory       = (pte_t *)vmalloc();
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

void vm_pae_create_pdpt_cache(boot_alloc_t *boot_alloc) {
    slab_cache_init(
            &pdpt_cache,
            "vm_pae_pdpt_cache",
            sizeof(pdpt_t),
            sizeof(pdpt_t),
            NULL,
            NULL,
            SLAB_DEFAULTS,
            boot_alloc);
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

addr_space_t *vm_pae_create_initial_addr_space(boot_alloc_t *boot_alloc) {

    unsigned int idx;
    
    /* Allocate initial PDPT. PDPT must be 32-byte aligned. */
    initial_pdpt = boot_heap_alloc(boot_alloc, pdpt_t, 32);
    
    /* We want the pre-allocated kernel page tables to be contiguous. For this
     * reason, we allocate the page directories first, and then the page tables.
     *
     * This function allocates pages in this order:
     *      +-------...--------+-------...------+
     *      |  pre-allocated   |  pre-allocated |
     *      |      kernel      |     kernel     |
     *      | page directories |  page tables   |
     *      +-------...--------+-------...------+
     * */

    clear_pdpt(initial_pdpt);

    const unsigned int last_idx = pdpt_offset_of((addr_t)KERNEL_PREALLOC_LIMIT - 1);

    for(idx = pdpt_offset_of((addr_t)KLIMIT); idx <= last_idx; ++idx) {
        pte_t *const pdpte      = &initial_pdpt->pd[idx];
        pte_t *page_directory   = (pte_t *)boot_page_alloc_early(boot_alloc);

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

        vm_init_initial_page_directory(
                page_directory,
                boot_alloc,
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
