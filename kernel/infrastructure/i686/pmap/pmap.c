/*
 * Copyright (C) 2019-2026 Philippe Aubertin.
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

#include <jinue/shared/asm/errno.h>
#include <jinue/shared/asm/mman.h>
#include <kernel/domain/alloc/page_alloc.h>
#include <kernel/domain/alloc/vmalloc.h>
#include <kernel/domain/entities/descriptor.h>
#include <kernel/domain/entities/object.h>
#include <kernel/domain/entities/process.h>
#include <kernel/domain/services/panic.h>
#include <kernel/infrastructure/i686/isa/instrs.h>
#include <kernel/infrastructure/i686/isa/regs.h>
#include <kernel/infrastructure/i686/memory/pages.h>
#include <kernel/infrastructure/i686/pmap/pmap.h>
#include <kernel/infrastructure/i686/pmap/private.h>
#include <kernel/infrastructure/i686/boot_alloc.h>
#include <kernel/infrastructure/i686/cpuinfo.h>
#include <kernel/infrastructure/i686/percpu.h>
#include <kernel/infrastructure/elf.h>
#include <kernel/interface/i686/boot.h>
#include <kernel/machine/pmap.h>
#include <kernel/utils/utils.h>
#include <sys/elf.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* This file contains memory management code that is independent of whether
 * Physical Address Extension (PAE) is enabled or not. The PAE code is in
 * pae.c. Non-PAE code is in nopae.c.
 *
 * Type "pte_t *" is a pointer on a page table or page directory entry. Such
 * pointers might point to a 32-bit (non-PAE) entry or a 64-bit (PAE) entry and,
 * for this reason, should not be referenced in this file. Page table/page
 * directory entries should only be manipulated through the PAE or non-PAE
 * functions, as appropriate depending on whether PAE is enabled or not. */

/** Kernel page tables
 *
 * During kernel initialization, kernel page tables are pre-allocated
 * sequentially so, in essence, this pointer points to one big page table that
 * covers the whole part of the address space that belongs to the kernel. */
pte_t *kernel_page_tables;

/** Number of entries per page tables
 *
 * 1024 (4kB / 4 bytes per entry) if PAE is disabled.
 *  512 (4kB / 8 bytes per entry) if PAE is enabled. */
size_t entries_per_page_table;

/** true if PAE is enabled, false otherwise */
bool pgtable_format_pae;

/** mask of valid page frame number bits */
uint64_t page_frame_number_mask;

/** First address space created during kernel initialization.
 *
 * This address space is used as a template for kernel page tables/directories
 * when creating new address spaces. */
static addr_space_t initial_addr_space;

/**
 * Get page table entry (PTE) at specified entry offset from specified PTE
 *
 * @param pte base page table entry (e.g. the beginning of a page table)
 * @param offset entry offset
 * @return PTE at specified offset
 */
static pte_t *get_pte_with_offset(pte_t *pte, unsigned int offset) {
    if(pgtable_format_pae) {
        return pae_get_pte_with_offset(pte, offset);
    }
    else {
        return nopae_get_pte_with_offset(pte, offset);
    }
}

/**
 * Get page table entry (PTE) at specified entry offset from specified PTE
 *
 * This function does the same thing as get_pte_with_offset() except it expects
 * and returns a const pointer.
 *
 * @param pte base page table entry (e.g. the beginning of a page table)
 * @param offset entry offset
 * @return PTE at specified offset
 */
static inline const pte_t *get_pte_with_offset_const(
        const pte_t     *pte,
        unsigned int     offset) {

    return get_pte_with_offset((pte_t *)pte, offset);
}

/**
 * Get entry offset of specified virtual address within page table
 *
 * @param addr virtual address
 * @return entry offset of address within page table
 */
static unsigned int page_table_offset_of(const void *addr) {
    if(pgtable_format_pae) {
        return pae_page_table_offset_of(addr);
    }
    else {
        return nopae_page_table_offset_of(addr);
    }
}

/**
 * Get entry offset of specified virtual address within page directory
 *
 * @param addr virtual address
 * @return entry offset of address within page directory
 */
static unsigned int page_directory_offset_of(const void *addr) {
    if(pgtable_format_pae) {
        return pae_page_directory_offset_of(addr);
    }
    else {
        return nopae_page_directory_offset_of(addr);
    }
}

/**
 * Copy a page table or page directory entry
 *
 * @param dest destination page table/directory entry
 * @param src source page table/directory entry
 */
static void copy_pte(pte_t *dest, const pte_t *src) {
    if(pgtable_format_pae) {
        pae_copy_pte(dest, src);
    }
    else {
        nopae_copy_pte(dest, src);
    }
}

/**
 * Copy an array of page table or page directory entries
 *
 * @param dest destination array
 * @param src source array
 * @param n number of entries to copy
 */
void copy_ptes(pte_t *dest, const pte_t *src, int n) {
    for(int idx = 0; idx < n; ++idx) {
        copy_pte(
                get_pte_with_offset(dest, idx),
                get_pte_with_offset_const(src, idx));
    }
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
static void set_pte(pte_t *pte, paddr_t paddr, uint64_t flags) {
    if(pgtable_format_pae) {
        pae_set_pte(pte, paddr, flags);
    }
    else {
        nopae_set_pte(pte, (uint32_t)paddr, flags);
    }
}

/**
 * Clear page table or page directory entry
 *
 * Once clear, the entry no longer refers to anything and is not considered
 * present in memory.
 *
 * @param pte page table or page directory entry
 */
static void clear_pte(pte_t *pte) {
    if(pgtable_format_pae) {
        pae_clear_pte(pte);
    }
    else {
        nopae_clear_pte(pte);
    }
}

/**
 * Clear and array of page table or page directory entries
 *
 * Once clear, the entries no longer refer to anything and are not considered
 * present in memory.
 *
 * @param pte page table or page directory entry array
 * @param n number of entries to clear
 */
void clear_ptes(pte_t *pte, int n) {
    for(int idx = 0; idx < n; ++idx) {
        clear_pte(get_pte_with_offset(pte, idx));
    }
}

/**
 * Get the physical address set in a page table or page directory entry
 *
 * @param pte page table or page directory entry array
 * @return physical address
 */
static paddr_t get_pte_paddr(const pte_t *pte) {
    if(pgtable_format_pae) {
        return pae_get_pte_paddr(pte);
    }
    else {
        return nopae_get_pte_paddr(pte);
    }
}

/**
 * Initialize virtual memory management
 *
 * @param bootinfo boot information structure
 */
void pmap_init(const bootinfo_t *bootinfo) {
    kernel_page_tables              = bootinfo->page_tables;
    initial_addr_space.cr3          = bootinfo->cr3;
    pgtable_format_pae              = cpu_has_feature(CPU_FEATURE_PAE);

    if(pgtable_format_pae) {
        initial_addr_space.top_level.pdpt = (pdpt_t *)PHYS_TO_VIRT_AT_16MB(bootinfo->cr3);
    } else {
        initial_addr_space.top_level.pd = bootinfo->page_directory;
    }

    entries_per_page_table  = pgtable_format_pae ? PAE_PAGE_TABLE_PTES :  NOPAE_PAGE_TABLE_PTES;
    page_frame_number_mask  = ((UINT64_C(1) << cpu_phys_addr_width()) - 1) & (~PAGE_MASK);

    /* Enable global pages */
    if(cpu_has_feature(CPU_FEATURE_PGE)) {
        set_cr4(get_cr4() | X86_CR4_PGE);
    }
}

static pte_t *clone_first_kernel_page_directory(void) {
    pte_t *pd_template;

    if(pgtable_format_pae) {
        pd_template = pae_lookup_page_directory(
                &initial_addr_space,
                (void *)JINUE_KLIMIT,
                false,
                NULL);
    }
    else {
        pd_template = nopae_lookup_page_directory(&initial_addr_space);
    }

    assert(pd_template != NULL);

    unsigned int klimit_offset = page_directory_offset_of((void *)JINUE_KLIMIT);

    if(klimit_offset == 0) {
        /* If the first kernel page directory is completely used by the kernel,
         * it is shared between address spaces instead of being cloned. */
        return pd_template;
    }

    pte_t *page_directory = page_alloc();

    if(page_directory != NULL) {
        clear_ptes(page_directory, klimit_offset);

        copy_ptes(
                get_pte_with_offset(page_directory, klimit_offset),
                get_pte_with_offset(pd_template, klimit_offset),
                entries_per_page_table - klimit_offset);
    }

    return page_directory;
}

static void free_first_kernel_page_directory(pte_t *page_directory) {
    unsigned int klimit_offset = page_directory_offset_of((void *)JINUE_KLIMIT);

    /* If the first kernel page directory is completely used by the kernel,
     * it is shared between address spaces instead of being cloned. */
    if(page_directory != NULL && klimit_offset != 0) {
        page_free(page_directory);
    }
}

bool pmap_create_addr_space(addr_space_t *addr_space) {
    pte_t *page_directory = clone_first_kernel_page_directory();

    if(page_directory == NULL) {
        return false;
    }

    if(!pgtable_format_pae) {
        nopae_create_addr_space(addr_space, page_directory);
        return true;
    }

    bool retval = pae_create_addr_space(addr_space, page_directory);

    if(!retval) {
        free_first_kernel_page_directory(page_directory);
    }

    return retval;
}

void destroy_page_directory(void *page_directory, unsigned int last_index) {
    for(unsigned int idx = 0; idx < last_index; ++idx) {
        pte_t *pte = get_pte_with_offset(page_directory, idx);

        if(pte_is_present(pte)) {
            page_free(
                    lookup_page_frame_address(
                            get_pte_paddr(pte)));
        }
    }

    page_free(page_directory);
}

void pmap_destroy_addr_space(addr_space_t *addr_space) {
    /** ASSERTION: address space must not be NULL */
    assert(addr_space != NULL);

    /** ASSERTION: the initial address space should not be destroyed */
    assert(addr_space != &initial_addr_space);

    /** ASSERTION: the current address space should not be destroyed */
    assert( addr_space != get_current_addr_space() );

    if(pgtable_format_pae) {
        pae_destroy_addr_space(addr_space);
    }
    else {
        nopae_destroy_addr_space(addr_space);
    }
}

void pmap_switch_addr_space(addr_space_t *addr_space) {
    set_cr3(addr_space->cr3);

    percpu_t *cpu_data = get_percpu_data();
    cpu_data->current_addr_space = addr_space;
}

/**
 * Lookup page table entry for specified kernel address
 *
 * @param addr kernel address to look up
 * @return pointer to page table entry
 */
static pte_t *lookup_kernel_page_table_entry(const void *addr) {
    /** ASSERTION: addr is aligned on a page boundary */
    assert( page_offset_of(addr) == 0 );

    /** ASSERTION: addr is a kernel pointer */
    assert( is_kernel_pointer(addr) );

    /* Fast path for global allocations by the kernel:
     *  - The page tables for the region just above JINUE_KLIMIT are pre-
     *    allocated during the creation of the address space, so there is no
     *    need to check if they are allocated or to allocate them;
     *  - The page tables are mapped contiguously at a known location
     *    during initialization, so no need for a table walk to find them;
     *  - The mappings for this region are global, so we don't need to
     *    specify an address space. */
    return get_pte_with_offset(
        kernel_page_tables,
        page_number_of((uintptr_t)addr - JINUE_KLIMIT));
}

/**
 * Lookup a page table for a specified userspace address and address space
 *
 * If the create_as_needed argument is true, new page tables are allocated as
 * needed, and NULL is only returned if allocation of new page tables fail.
 * If the create_as_needed argument is false, NULL is returned if there is
 * currently no page table entry for the specified address and address space.
 *
 * If a new page table is allocated (when create_as_needed is true), CR3 may
 * need to be reloaded. If it's the case, the boolean pointed to by reload_cr3
 * is set to true. Initially setting this boolean to false is the caller's
 * responsibility.
 *
 * reload_cr3 must not be NULL if create_as_needed is true but is ignored and
 * can be set to NULL if create_as_needed is false.
 *
 * @param addr_space address space in which the address is looked up.
 * @param addr userspace address to look up
 * @param create_as_needed whether a page table is allocated if it does not exist
 * @param reload_cr3 (out) set to true if CR3 needs to be reloaded
 * @return pointer to page table on success, NULL otherwise
 */
static pte_t *lookup_userspace_page_table(
        addr_space_t    *addr_space,
        const void      *addr,
        bool             create_as_needed,
        bool            *reload_cr3) {

    pte_t *page_directory;

    /** ASSERTION: addr_space cannot be NULL for non-global mappings */
    assert(addr_space != NULL);

    /** ASSERTION: addr is aligned on a page boundary */
    assert( page_offset_of(addr) == 0 );

    /** ASSERTION: addr is a userspace pointer */
    assert( is_userspace_pointer(addr) );

    if(pgtable_format_pae) {
        page_directory = pae_lookup_page_directory(
            addr_space,
            addr,
            create_as_needed,
            reload_cr3
        );
    }
    else {
        page_directory = nopae_lookup_page_directory(addr_space);
    }
    
    if(page_directory == NULL) {
        /* no page directory */
        return NULL;
    }

    /* lookup page directory entry */
    pte_t *pde = get_pte_with_offset(page_directory, page_directory_offset_of(addr));

    if(pte_is_present(pde)) {
        return lookup_page_frame_address(get_pte_paddr(pde));
    }

    if(! create_as_needed) {
        return NULL;
    }

    pte_t *page_table = page_alloc();

    if(page_table != NULL) {
        clear_page(page_table);

        /* Do not add X86_PTE_GLOBAL here. X86_PTE_GLOBAL is specified for page
         * table entries, not page directory entries. */
        uint64_t flags = X86_PTE_READ_WRITE | X86_PTE_PRESENT;

        if(is_userspace_pointer(addr)) {
            flags |= X86_PTE_USER;
        }

        set_pte(pde, machine_lookup_kernel_paddr(page_table), flags);
    }

    return page_table;
}

/**
 * Translate architecture-independent protection flags to architecture-dependent flags
 *
 * Argument should be either JINUE_PROT_NONE (no access) or any combination of
 * JINUE_PROT_READ, JINUE_PROT_WRITE and/or JINUE_PROT_EXEC.
 *
 * Return value is a combination of architecture-dependent flags appropriate to
 * be set directly in a page table entry (i.e. the X86_PTE_... constants).
 *
 * @param prot architecture-independent protection flags
 * @return architecture-dependent flags
 */
static uint64_t map_page_access_flags(int prot) {
    const int rwe_mask = JINUE_PROT_READ | JINUE_PROT_WRITE | JINUE_PROT_EXEC;

    if(! (prot & rwe_mask)) {
        return X86_PTE_PROT_NONE;
    }

    int mapped_flags = X86_PTE_PRESENT;

    if(prot & JINUE_PROT_WRITE) {
        mapped_flags |= X86_PTE_READ_WRITE;
    }

    if(! (prot & JINUE_PROT_EXEC)) {
        mapped_flags |= X86_PTE_NX;
    }

    return mapped_flags;
}

/**
 * Establish a kernel virtual memory mapping for a single page.
 *
 * Kernel page tables are pre-allocated during kernel initialization so this is
 * guaranteed to succeed. There is also no need to specify an address space
 * since kernel mappings are global.
 *
 * @param vaddr virtual address of mapping
 * @param paddr address of page frame
 * @param prot protections flags
 */
void machine_map_kernel(addr_t addr, size_t size, paddr_t paddr, int prot) {
    /** ASSERTION: we assume vaddr is aligned on a page boundary */
    assert( page_offset_of(addr) == 0 );

    pte_t *pte = lookup_kernel_page_table_entry(addr);

    assert(pte != NULL);

    for(size_t offset = 0; offset < size; offset += PAGE_SIZE) {
        set_pte(
            get_pte_with_offset(pte, PAGE_NUMBER(offset)),
            paddr + offset,
            map_page_access_flags(prot) | X86_PTE_GLOBAL
        );

        invlpg(addr + offset);
    }
}

/**
 * Establish a userspace virtual memory mapping.
 *
 * Page tables are allocated as needed. If an allocation fails, this function
 * returns false to indicate failure.
 *
 * @param process process in which to map
 * @param vaddr start virtual address of mapping
 * @param paddr start address in physical memory
 * @param length length of mapping
 * @param prot protection flags
 * @return true on success, false on page table allocation error
 */
bool machine_map_userspace(
        process_t       *process,
        addr_t           addr,
        size_t           size,
        paddr_t          paddr,
        int              prot) {

    /** ASSERTION: we assume vaddr is aligned on a page boundary */
    assert( page_offset_of(addr) == 0 );

    addr_space_t *addr_space = &process->addr_space;
    bool needs_invalidation = (get_cr3() == addr_space->cr3);

    int pte_index   = page_table_offset_of(addr);

    bool reload_cr3 = false;
    pte_t *pte      = lookup_userspace_page_table(addr_space, addr, true, &reload_cr3);

    if(pte == NULL) {
        return false;
    }

    for(size_t offset = 0; offset < size; offset += PAGE_SIZE) {
        if(pte_index >= entries_per_page_table) {
            pte = lookup_userspace_page_table(
                addr_space,
                addr + offset,
                true,
                &reload_cr3
            );

            if(pte == NULL) {
                return false;
            }

            pte_index = 0;
        }

        set_pte(
            get_pte_with_offset(pte, pte_index),
            paddr + offset,
            map_page_access_flags(prot) | X86_PTE_USER
        );

        if(needs_invalidation && !reload_cr3) {
            invlpg(addr + offset);
        }

        ++pte_index;
    }

    if(needs_invalidation && reload_cr3) {
        set_cr3(get_cr3());
    }

    return true;
}

/**
 * Unmap a kernel page from virtual memory.
 *
 * There is no need to specify an address space since kernel mappings are global.
 *
 * This function does not perform any page table deallocation.
 *
 * @param addr address of page to unmap
 */
void machine_unmap_kernel(addr_t addr, size_t size) {
    /** ASSERTION: addr is aligned on a page boundary */
    assert( page_offset_of(addr) == 0 );

    pte_t *pte = lookup_kernel_page_table_entry(addr);

    assert(pte != NULL);

    for(size_t offset = 0; offset < size; offset += PAGE_SIZE) {
        clear_pte( get_pte_with_offset(pte, PAGE_NUMBER(offset)) );

        invlpg(addr + offset);
    }
}

/**
 * Look up the physical address of an existing kernel-mapped page frame.
 *
 * @param addr virtual address of kernel page
 * @return physical address of page frame
 */
paddr_t machine_lookup_kernel_paddr(const void *addr) {
    pte_t *pte = lookup_kernel_page_table_entry(addr);

    assert(pte != NULL && pte_is_present(pte));

    return get_pte_paddr(pte);
}
