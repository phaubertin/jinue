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

#include <kernel/i686/boot.h>
#include <kernel/i686/boot_alloc.h>
#include <kernel/i686/cpu.h>
#include <kernel/i686/memory.h>
#include <kernel/i686/vm_private.h>
#include <kernel/i686/x86.h>
#include <kernel/page_alloc.h>
#include <kernel/panic.h>
#include <kernel/slab.h>
#include <kernel/util.h>
#include <kernel/vmalloc.h>
#include <assert.h>
#include <string.h>

/* This file contains Physical Address Extension (PAE) memory management code.
 * Non-PAE code is located in vm_x86.c. Virtual memory management code
 * independent of PAE is located in vm.c. */

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

static pdpt_t *initial_pdpt;

static uint64_t page_frame_number_mask;

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
 * Initialize boot-time page table for identity mapping at 1MB
 *
 * @param page_table_1mb page table for mapping at 0x100000 (1MB)
 *
 * */
static void initialize_boot_mapping_at_1mb(pte_t *page_table_1mb) {
    vm_initialize_page_table_linear(
            page_table_1mb,
            0,
            X86_PTE_READ_WRITE,
            PAGE_TABLE_ENTRIES);
}

/**
 * Initialize boot-time page tables for identity mapping at 16MB
 *
 * @param page_table_16mb first page table for mapping at 0x1000000 (16MB)
 *
 * */
static void initialize_boot_mapping_at_16mb(
        pte_t               *page_table_16mb,
        const boot_info_t   *boot_info) {

    size_t image_size = (char *)boot_info->image_top - (char *)boot_info->image_start;
    size_t image_pages = image_size / PAGE_SIZE;

    /* map kernel image read only */
    pte_t *next_pte = vm_initialize_page_table_linear(
            page_table_16mb,
            MEMORY_ADDR_16MB,
            0,
            image_pages);

    /* map remaining of region read/write */
    vm_initialize_page_table_linear(
            next_pte,
            MEMORY_ADDR_16MB + image_size,
            X86_PTE_READ_WRITE,
            BOOT_PTES_AT_16MB - image_pages);
}

/**
 * Initialize boot-time page table for mapping at KLIMIT
 *
 * @param page_table_klimit page table for mapping at KLIMIT
 *
 * */
static void initialize_boot_mapping_at_klimit(
        pte_t               *page_table_klimit,
        const boot_info_t   *boot_info) {

    uint32_t size_at_16mb = (uint32_t)boot_info->page_table_1mb - MEMORY_ADDR_1MB;
    uint32_t num_entries_at_16mb = size_at_16mb / PAGE_SIZE;

    size_t image_size = (char *)boot_info->image_top - (char *)boot_info->image_start;
    size_t image_pages = image_size / PAGE_SIZE;

    /* map kernel image read only */
    pte_t *next_pte_after_image = vm_initialize_page_table_linear(
            page_table_klimit,
            MEMORY_ADDR_16MB,
            0,
            image_pages);

    /* map kernel data segment */
    size_t offset = ((uintptr_t)boot_info->data_start - KLIMIT) / PAGE_SIZE;

    vm_initialize_page_table_linear(
            vm_pae_get_pte_with_offset(page_table_klimit, offset),
            boot_info->data_physaddr + MEMORY_ADDR_16MB - MEMORY_ADDR_1MB,
            X86_PTE_READ_WRITE,
            boot_info->data_size / PAGE_SIZE);

    /* map rest of region read/write */
    vm_initialize_page_table_linear(
            next_pte_after_image,
            MEMORY_ADDR_16MB + image_size,
            X86_PTE_READ_WRITE,
            num_entries_at_16mb - image_pages);
}

/**
 * Initialize boot-time page directory for identity mappings at 1MB and 16MB
 *
 * @param low_page_directory first page directory
 * @param page_table_1mb page table for mapping at 0x100000 (1MB)
 * @param page_table_16mb first page table for mapping at 0x1000000 (16MB)
 *
 * */
static void initialize_boot_low_page_directory(
        pte_t           *low_page_directory,
        const pte_t     *page_table_1mb,
        const pte_t     *page_table_16mb) {

    vm_pae_set_pte(
            low_page_directory,
            (uintptr_t)page_table_1mb,
            X86_PTE_READ_WRITE | X86_PTE_PRESENT);

    vm_initialize_page_table_linear(
            vm_pae_get_pte_with_offset(
                    low_page_directory,
                    vm_pae_page_directory_offset_of((addr_t)MEMORY_ADDR_16MB)),
            (uintptr_t)page_table_16mb,
            X86_PTE_READ_WRITE,
            BOOT_PTES_AT_16MB / PAGE_TABLE_ENTRIES);
}

/**
 * Initialize boot-time page directory for mapping at KLIMIT
 *
 * @param page_directory_klimit page directory for mapping at KLIMIT
 * @param page_table_klimit page table for mapping at KLIMIT
 *
 * */
static void initialize_boot_page_directory_klimit(
        pte_t           *page_directory_klimit,
        const pte_t     *page_table_klimit) {

    vm_pae_set_pte(
            vm_pae_get_pte_with_offset(
                    page_directory_klimit,
                    vm_pae_page_directory_offset_of((addr_t)KLIMIT)),
            (uintptr_t)page_table_klimit,
            X86_PTE_READ_WRITE | X86_PTE_PRESENT);
}

/**
 * Initialize boot-time Page Directory Pointer Table (PDPT)
 *
 * @param pdpt Page Directory Pointer Table (PDPT)
 * @param low_page_directory first page directory
 * @param page_directory_klimit page directory for mapping at KLIMIT
 *
 * */
static void initialize_boot_pdpt(
        pdpt_t          *pdpt,
        const pte_t     *low_page_directory,
        const pte_t     *page_directory_klimit) {

    vm_pae_set_pte(
            &pdpt->pd[0],
            (uintptr_t)low_page_directory,
            X86_PTE_PRESENT);

    vm_pae_set_pte(
            &pdpt->pd[pdpt_offset_of((addr_t)KLIMIT)],
            (uintptr_t)page_directory_klimit,
            X86_PTE_PRESENT);
}

/**
 * Allocate and initialize boot-time PAE page tables
 *
 * The 32-bit setup code has set up initial page tables with the following three
 * mappings:
 *
 * 1) First two megabytes of memory are identity mapped.
 * 2) Some megabytes of memory at 0x1000000 (16M) are also identity mapped.
 * 3) The kernel image and following stack, heap and other initial allocations
 *    are mapped at KLIMIT.
 *
 * See doc/layout.md for detail on mappings.
 *
 * This function creates new paging structures with the exact same mappings as
 * the current ones but in PAE format.
 *
 * Mappings 1) and 3) are 2MB or less and so require one page table each.
 * Mapping 2 is a few megabytes and requires a few page tables.
 *
 * Mappings 1) and 2) are in the first page directory while mapping 3) is not,
 * so we need two page directories.
 *
 * @param boot_alloc state of the boot-time allocator
 * @param boot_info boot information structure
 *
 * */
static pdpt_t *initialize_boot_page_tables(
        boot_alloc_t        *boot_alloc,
        const boot_info_t   *boot_info) {

    /* First mapping */
    pte_t *page_table_1mb = boot_page_alloc(boot_alloc);

    initialize_boot_mapping_at_1mb(page_table_1mb);

    /* Second mapping (a few page tables) */
    pte_t *page_table_16mb = boot_page_alloc_n(
            boot_alloc,
            BOOT_PTES_AT_16MB / PAGE_TABLE_ENTRIES);

    initialize_boot_mapping_at_16mb(page_table_16mb, boot_info);

    /* Third mapping */
    pte_t *page_table_klimit = boot_page_alloc(boot_alloc);

    initialize_boot_mapping_at_klimit(page_table_klimit, boot_info);

    /* Page directory for first two mappings */
    pte_t *low_page_directory = boot_page_alloc(boot_alloc);

    initialize_boot_low_page_directory(
            low_page_directory,
            page_table_1mb,
            page_table_16mb);

    /* Page directory for third mapping */
    pte_t *page_directory_klimit = boot_page_alloc(boot_alloc);

    initialize_boot_page_directory_klimit(
            page_directory_klimit,
            page_table_klimit);

    /* Initialize PDPT */
    pdpt_t *pdpt = boot_heap_alloc(boot_alloc, pdpt_t, sizeof(pdpt_t));

    initialize_boot_pdpt(pdpt, low_page_directory, page_directory_klimit);

    return pdpt;
}

/**
 * Enable Physical Address Extension (PAE)
 *
 * Allocate and initialize boot-time page tables, then enable PAE.
 *
 * @param boot_alloc state of the boot-time allocator
 * @param boot_info boot information structure
 *
 * */
void vm_pae_enable(boot_alloc_t *boot_alloc, const boot_info_t *boot_info) {
    pgtable_format_pae      = true;
    entries_per_page_table  = VM_PAE_PAGE_TABLE_PTES;
    page_frame_number_mask  = ((UINT64_C(1) << cpu_info.maxphyaddr) - 1) & (~PAGE_MASK);

    pdpt_t *pdpt = initialize_boot_page_tables(boot_alloc, boot_info);

    x86_enable_pae(PTR_TO_PHYS_ADDR_AT_16MB(pdpt));

    /* Enable support for NX/XD bit */
    uint64_t msrval  = rdmsr(MSR_EFER);
    msrval |= MSR_FLAG_EFER_NXE;
    wrmsr(MSR_EFER, msrval);
}

void vm_pae_create_initial_addr_space(
        addr_space_t    *address_space,
        pte_t           *page_directories,
        boot_alloc_t    *boot_alloc) {

    /* Allocate initial PDPT. PDPT must be 32-byte aligned. */
    initial_pdpt = boot_heap_alloc(boot_alloc, pdpt_t, 32);

    int offset = pdpt_offset_of((void *)KLIMIT);
    vm_initialize_page_table_linear(
            &initial_pdpt->pd[offset],
            (uintptr_t)page_directories,
            X86_PTE_PRESENT,
            PDPT_ENTRIES - offset);

    address_space->top_level.pdpt   = initial_pdpt;
    address_space->cr3              = VIRT_TO_PHYS_AT_16MB(initial_pdpt);
}

bool vm_pae_create_addr_space(addr_space_t *addr_space, pte_t *first_page_directory) {
    /* Create a PDPT for the new address space */
    pdpt_t *pdpt = slab_cache_alloc(&pdpt_cache);

    if(pdpt == NULL) {
        return false;
    }

    clear_pdpt(pdpt);

    unsigned int klimit_offset = pdpt_offset_of((void *)KLIMIT);

    vm_pae_set_pte(
            &pdpt->pd[klimit_offset],
            vm_lookup_kernel_paddr(first_page_directory),
            X86_PTE_PRESENT);

    for(int idx = klimit_offset + 1; idx < PDPT_ENTRIES; ++idx) {
        vm_pae_copy_pte(&pdpt->pd[idx], &initial_pdpt->pd[idx]);
    }

    /* Lookup the physical address of the page where the PDPT resides. */
    kern_paddr_t pdpt_page_paddr = vm_lookup_kernel_paddr((addr_t)page_address_of(pdpt));

    /* physical address of PDPT */
    kern_paddr_t pdpt_paddr = pdpt_page_paddr | page_offset_of(pdpt);

    addr_space->top_level.pdpt  = pdpt;
    addr_space->cr3             = pdpt_paddr;

    return true;
}

void vm_pae_destroy_addr_space(addr_space_t *addr_space) {
    pdpt_t *pdpt = addr_space->top_level.pdpt;

    for(unsigned int idx = 0; idx < pdpt_offset_of((void *)KLIMIT); ++idx) {
        pte_t *pdpte = &pdpt->pd[idx];

        if(pte_is_present(pdpte)) {
            vm_destroy_page_directory(
                    memory_lookup_page(vm_pae_get_pte_paddr(pdpte)),
                    entries_per_page_table);
        }
    }

    /* Depending on the value of KLIMIT, there are two possible cases with
     * regards to the first kernel page directory, i.e. the first page directory
     * that links kernel page tables:
     *
     * 1. That page directory links *only* kernel page tables, i.e. the page
     *    directory entry for address KLIMIT is the first entry of that page
     *    directory. In this case, this page directory is shared by all address
     *    spaces and we must do nothing here. We must not free the kernel page
     *    tables it links to nor the page directory itself.
     *
     * 2. That page directory is split between userspace and the kernel. It
     *    starts with userspace entries, followed by kernel entries, because the
     *    entry for address KLIMIT is at some non-zero offset within this page
     *    directory. In this case, we must free the userspace page tables, but
     *    not the kernel page tables, and then free the page directory. */
    unsigned int klimit_offset = vm_pae_page_directory_offset_of((void *)KLIMIT);

    if(klimit_offset > 0) {
        pte_t *pdpte = &pdpt->pd[pdpt_offset_of((void *)KLIMIT)];

        assert(pte_is_present(pdpte));

        vm_destroy_page_directory(
                memory_lookup_page(vm_pae_get_pte_paddr(pdpte)),
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
pte_t *vm_pae_lookup_page_directory(
        addr_space_t    *addr_space,
        void            *addr,
        bool             create_as_needed,
        bool            *reload_cr3) {

    /** ASSERTION: addr_space must not be NULL */
    assert(addr_space != NULL);

    /** ASSERTION: reload_cr3 can only be NULL if create_as_needed is false */
    assert(reload_cr3 != NULL || !create_as_needed);

    pdpt_t *pdpt    = addr_space->top_level.pdpt;
    pte_t  *pdpte   = &pdpt->pd[pdpt_offset_of(addr)];
    
    if(pte_is_present(pdpte)) {
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
 *
 */
unsigned int vm_pae_page_table_offset_of(void *addr) {
    return PAGE_TABLE_OFFSET_OF(addr);
}

/**
 * Get entry offset of specified virtual address within page directory
 *
 * @param addr virtual address
 * @return entry offset of address within page directory
 *
 */
unsigned int vm_pae_page_directory_offset_of(void *addr) {
    return PAGE_DIRECTORY_OFFSET_OF(addr);
}

/**
 * Get page table entry (PTE) at specified entry offset from specified PTE
 *
 * @param pte base page table entry (e.g. the beginning of a page table)
 * @param offset entry offset
 * @return PTE at specified offset
 *
 */
pte_t *vm_pae_get_pte_with_offset(pte_t *pte, unsigned int offset) {
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
 *
 */
void vm_pae_set_pte(pte_t *pte, uint64_t paddr, uint64_t flags) {
    assert((paddr & ~page_frame_number_mask) == 0);
    pte->entry = paddr | flags;
}

/**
 * Set protection and other flags on specified page table entry
 *
 * The appropriate flags for this function are the architecture-dependent flags,
 * i.e. those defined by the X86_PTE_... constants. See map_page_access_flags()
 * for additional context.
 *
 * @param pte page table entry
 * @param pte flags flags
 *
 */
void vm_pae_set_pte_flags(pte_t *pte, uint64_t flags) {
    pte->entry = (pte->entry & page_frame_number_mask) | flags;
}

/**
 * Get the physical address set in a page table or page directory entry
 *
 * @param pte page table or page directory entry array
 * @return physical address
 *
 */
uint64_t vm_pae_get_pte_paddr(const pte_t *pte) {
    return (pte->entry & page_frame_number_mask);
}

/**
 * Clear page table or page directory entry
 *
 * Once clear, the entry no longer refers to anything and is not considered
 * present in memory.
 *
 * @param pte page table or page directory entry
 *
 */
void vm_pae_clear_pte(pte_t *pte) {
    pte->entry = 0;
}

/**
 * Copy a page table or page directory entry
 *
 * @param dest destination page table/directory entry
 * @param src source page table/directory entry
 *
 */
void vm_pae_copy_pte(pte_t *dest, const pte_t *src) {
    dest->entry = src->entry;
}

void vm_pae_create_pdpt_cache(void) {
    slab_cache_init(
            &pdpt_cache,
            "pae_pdpt",
            sizeof(pdpt_t),
            sizeof(pdpt_t),
            NULL,
            NULL,
            SLAB_DEFAULTS);
}
