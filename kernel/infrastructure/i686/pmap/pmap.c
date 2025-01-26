/*
 * Copyright (C) 2019-2024 Philippe Aubertin.
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
#include <kernel/infrastructure/i686/pmap/pmap.h>
#include <kernel/infrastructure/i686/pmap/private.h>
#include <kernel/infrastructure/i686/boot_alloc.h>
#include <kernel/infrastructure/i686/memory.h>
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
 * Set protection and other flags on specified page table entry
 *
 * The appropriate flags for this function are the architecture-dependent flags,
 * i.e. those defined by the X86_PTE_... constants. See map_page_access_flags()
 * for additional context.
 *
 * @param pte page table entry
 * @param pte flags flags
 */
static void set_pte_flags(pte_t *pte, uint64_t flags) {
    if(pgtable_format_pae) {
        pae_set_pte_flags(pte, flags);
    }
    else {
        nopae_set_pte_flags(pte, flags);
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
 * Initialize virtual memory management to not use PAE
 *
 * During initialization, the kernel either calls this function or calls
 * pae_enable() to enable PAE.
 */
void pmap_set_no_pae(void) {
    pgtable_format_pae      = false;
    entries_per_page_table  = NOPAE_PAGE_TABLE_PTES;
}

/**
 * Initialize consecutive page table entries to map consecutive page frames
 *
 * @param page_table first page table entry
 * @param start_paddr start physical address
 * @param flags page table entry flags
 * @param num_entries number of entries to initialize
 * @return first page table entry after affected ones
 */
pte_t *initialize_page_table_linear(
        pte_t       *page_table,
        uint64_t     start_paddr,
        uint64_t     flags,
        int          num_entries) {

    uint64_t paddr  = start_paddr;

    for(int idx = 0; idx < num_entries; ++idx) {
        set_pte(
                get_pte_with_offset(page_table, idx),
                paddr,
                flags | X86_PTE_PRESENT);

        paddr += PAGE_SIZE;
    }

    return get_pte_with_offset(page_table, num_entries);
}

/**
 * Write protect the kernel image at address 0x1000000 (16MB)
 *
 * This function is called during initialization after the kernel image has been
 * moved from address 0x100000 (1MB) to address 0x1000000 (16MB) to ensure the
 * new copy is read only.
 *
 * @param bootinfo boot information structure
 */
void pmap_write_protect_kernel_image(const bootinfo_t *bootinfo) {
    size_t image_size = (char *)bootinfo->image_top - (char *)bootinfo->image_start;
    size_t image_pages = image_size / PAGE_SIZE;

    for(int idx = 0; idx < image_pages; ++idx) {
        set_pte_flags(
                get_pte_with_offset(bootinfo->page_table_16mb, idx),
                X86_PTE_PRESENT); /* read only */
    }

    set_cr3((uintptr_t)bootinfo->page_directory);
}

static void initialize_initial_page_tables(
        pte_t               *page_tables,
        Elf32_Ehdr          *ehdr,
        const bootinfo_t   *bootinfo) {

    size_t image_size  = (char *)bootinfo->image_top - (char *)bootinfo->image_start;
    size_t image_pages = image_size / PAGE_SIZE;

    /* map kernel image read only, not executable */
    pte_t *next_pte_after_image = initialize_page_table_linear(
            page_tables,
            MEMORY_ADDR_16MB,
            X86_PTE_GLOBAL | X86_PTE_NX,
            image_pages);

    /* map rest of region read/write */
    initialize_page_table_linear(
            next_pte_after_image,
            MEMORY_ADDR_16MB + image_size,
            X86_PTE_GLOBAL | X86_PTE_READ_WRITE | X86_PTE_NX,
            BOOT_SIZE_AT_16MB / PAGE_SIZE - image_pages);

    /* make kernel code segment executable */
    const Elf32_Phdr *phdr = elf_executable_program_header(ehdr);

    if(phdr == NULL) {
        panic("could not find kernel executable segment");
    }

    uintptr_t code_vaddr    = ALIGN_START((uintptr_t)phdr->p_vaddr, PAGE_SIZE);
    size_t code_size        = phdr->p_memsz + OFFSET_OF_PTR(phdr->p_vaddr, PAGE_SIZE);
    size_t code_offset      = (code_vaddr - KLIMIT) / PAGE_SIZE;

    initialize_page_table_linear(
            get_pte_with_offset(page_tables, code_offset),
            code_vaddr + MEMORY_ADDR_16MB - KLIMIT,
            X86_PTE_GLOBAL,
            code_size / PAGE_SIZE);

    /* map kernel data segment */
    size_t data_offset = ((uintptr_t)bootinfo->data_start - KLIMIT) / PAGE_SIZE;

    initialize_page_table_linear(
            get_pte_with_offset(page_tables, data_offset),
            bootinfo->data_physaddr + MEMORY_ADDR_16MB - MEMORY_ADDR_1MB,
            X86_PTE_GLOBAL | X86_PTE_READ_WRITE | X86_PTE_NX,
            bootinfo->data_size / PAGE_SIZE);
}

static void initialize_initial_page_directories(
        pte_t   *page_directories,
        pte_t   *page_tables,
        int      num_page_tables) {

    int offset = page_directory_offset_of((void *)KLIMIT);

    initialize_page_table_linear(
            get_pte_with_offset(page_directories, offset),
            (uintptr_t)page_tables,
            X86_PTE_READ_WRITE,
            num_page_tables);
}

addr_space_t *pmap_create_initial_addr_space(
        const exec_file_t   *kernel,
        boot_alloc_t        *boot_alloc,
        const bootinfo_t    *bootinfo) {
    
    Elf32_Ehdr *ehdr = kernel->start;

    /* Pre-allocate all the kernel page tables. */
    int num_pages           = (ADDR_4GB - KLIMIT) / PAGE_SIZE;
    int num_page_tables     = num_pages / entries_per_page_table;

    /* The number of entries in a pages table (page_table_entries) is also the
     * number of entries in a page directory. */
    int num_page_dirs       = ALIGN_END(num_page_tables, entries_per_page_table) / entries_per_page_table;

    /* allocate tables */
    pte_t *page_tables      = boot_page_alloc_n(boot_alloc, num_page_tables);
    pte_t *page_directories = boot_page_alloc_n(boot_alloc, num_page_dirs);

    initialize_initial_page_tables(page_tables, ehdr, bootinfo);
    initialize_initial_page_directories(page_directories, page_tables, num_page_tables);

    kernel_page_tables      = (pte_t *)PHYS_TO_VIRT_AT_16MB(page_tables);

    if(pgtable_format_pae) {
        pae_create_initial_addr_space(
                &initial_addr_space,
                page_directories,
                boot_alloc);
    }
    else {
        nopae_create_initial_addr_space(&initial_addr_space, page_directories);
    }

    return &initial_addr_space;
}

static pte_t *clone_first_kernel_page_directory(void) {
    pte_t *pd_template;

    if(pgtable_format_pae) {
        pd_template = pae_lookup_page_directory(
                &initial_addr_space,
                (void *)KLIMIT,
                false,
                NULL);
    }
    else {
        pd_template = nopae_lookup_page_directory(&initial_addr_space);
    }

    assert(pd_template != NULL);

    unsigned int klimit_offset = page_directory_offset_of((void *)KLIMIT);

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
    unsigned int klimit_offset = page_directory_offset_of((void *)KLIMIT);

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
                    memory_lookup_page(
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

static pte_t *pmap_lookup_page_table(
        addr_space_t    *addr_space,
        const void      *addr,
        bool             create_as_needed,
        bool            *reload_cr3) {

    pte_t *page_directory;

    /** ASSERTION: addr_space cannot be NULL for non-global mappings */
    assert(addr_space != NULL);

    if(pgtable_format_pae) {
        page_directory = pae_lookup_page_directory(
                addr_space,
                addr,
                create_as_needed,
                reload_cr3);
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
        return memory_lookup_page(get_pte_paddr(pde));
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
 * Lookup a page table entry for a specified address and address space.
 *
 * If the create_as_needed argument is true, new page tables are allocated as
 * needed, and NULL is only returned if allocation of new page tables fail.
 * If the create_as_needed argument is false, NULL is returned if there is
 * currently no page table entry for the specified address and address space.
 *
 * If a new page table is allocated (when create_as_needed is true), CR3 may
 * need to be reloaded. If it's the case, the boolean pointed to by reload_cr3
 * is set to true. Initially setting this boolean to false it the caller's
 * responsibility. The expected sequence of events is as follow:
 *
 * reload_cr3 must not be NULL if create_as_needed is true but is ignored and
 * can be set to NULL if create_as_needed is false.
 *
 * @param addr_space address space in which the address is looked up.
 * @param addr address to look up
 * @param create_as_needed whether a page table is allocated if it does not exist
 * @param reload_cr3 (out) set to true if CR3 needs to be reloaded
 * @return pointer to page table entry on success, NULL otherwise
 *
 * */
static pte_t *lookup_page_table_entry(
        addr_space_t    *addr_space,
        const void      *addr,
        bool             create_as_needed,
        bool            *reload_cr3) {

    /** ASSERTION: addr is aligned on a page boundary */
    assert( page_offset_of(addr) == 0 );

    if(is_kernel_pointer(addr)) {
        /* Fast path for global allocations by the kernel:
         *  - The page tables for the region just above KLIMIT are pre-allocated
         *    during the creation of the address space, so there is no need to
         *    check if they are allocated or to allocate them;
         *  - The page tables are mapped contiguously at a known location
         *    during initialization, so no need for a table walk to find them;
         *  - The mappings for this region are global, so we don't care
         *    about the specified address space.  */
        return get_pte_with_offset(
                kernel_page_tables,
                page_number_of((uintptr_t)addr - KLIMIT));
    }

    pte_t *page_table = pmap_lookup_page_table(
            addr_space,
            addr,
            create_as_needed,
            reload_cr3);

    if(page_table == NULL) {
        return NULL;
    }

    return get_pte_with_offset(page_table, page_table_offset_of(addr));
}

/**
 * Invalidate the mapping of a single page.
 *
 * If reload_cr3 is true, the invalidation is performed by reloading the CR3
 * control register. This is necessary when PAE is enabled when the PDPT was
 * just modified. If reload_cr3 is false, which is the common case, the invlpg
 * instruction is used instead.
 *
 * The invalidation is only performed is the specified address space is the
 * currently active one or if the mapping is a kernel mapping, since kernel
 * mappings are global.
 *
 * @param addr_space address space in which a mapping change was performed
 * @param addr virtual address of mapping change
 * @param reload_cr3 true if CR3 register must be reloaded
 */
static void invalidate_mapping(
        addr_space_t    *addr_space,
        void            *addr,
        bool             reload_cr3) {

    assert(addr_space != NULL || is_kernel_pointer(addr));

    uint32_t cr3 = get_cr3();

    /* Be careful with condition order here (short-circuit evaluation): if addr
     * is a kernel pointer, addr_space is allowed to be NULL, so we musn't
     * dereference it. */
    if(is_kernel_pointer(addr) || cr3 == addr_space->cr3) {
        if(reload_cr3) {
            set_cr3(cr3);
        }
        else {
            invlpg(addr);
        }
    }
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
 * Map a page frame (physical page) to a virtual memory page.
 *
 * This function is intended to be called by map_userspace_page()) and
 * machine_map_kernel_page(). Either of these functions should be called
 * elsewhere instead of calling this function directly.
 *
 * There is no need to specify an address space for kernel mappings since
 * kernel mappings are global.
 *
 * Page tables are allocated as needed. If an allocation fails, this function
 * returns false to indicate failure.
 *
 * @param addr_space address space in which to map, NULL for kernel mappings
 * @param vaddr virtual address of mapping
 * @param paddr address of page frame
 * @param flags flags used for mapping (see X86_PTE_... constants)
 * @return true on success, false on page table allocation error
 */
static bool map_page(
        addr_space_t    *addr_space,
        void            *vaddr,
        paddr_t          paddr,
        uint64_t         flags) {

    /** ASSERTION: we assume vaddr is aligned on a page boundary */
    assert( page_offset_of(vaddr) == 0 );
    
    bool reload_cr3 = false;
    pte_t *pte = lookup_page_table_entry(addr_space, vaddr, true, &reload_cr3);
    
    /* kernel page tables are pre-allocated so this should always succeed for
     * kernel mappings. */
    assert(pte != NULL || is_userspace_pointer(vaddr));
    
    if(pte == NULL) {
        return false;
    }

    set_pte(pte, paddr, flags);

    invalidate_mapping(addr_space, vaddr, reload_cr3);

    return true;
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
void machine_map_kernel_page(void *vaddr, paddr_t paddr, int prot) {
    assert(is_kernel_pointer(vaddr));
    map_page(NULL, vaddr, paddr, map_page_access_flags(prot) | X86_PTE_GLOBAL);
}

/**
 * Establish a userspace virtual memory mapping for a single page.
 *
 * Page tables are allocated as needed. If an allocation fails, this function
 * returns false to indicate failure.
 *
 * @param addr_space address space in which to map
 * @param vaddr virtual address of mapping
 * @param paddr address of page frame
 * @param prot protections flags
 * @return true on success, false on page table allocation error
 */
static bool map_userspace_page(
        addr_space_t    *addr_space,
        void            *vaddr,
        paddr_t          paddr,
        int              prot) {

    assert(is_userspace_pointer(vaddr));
    return map_page(addr_space, vaddr, paddr, map_page_access_flags(prot) | X86_PTE_USER);
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
        void            *vaddr,
        size_t           length,
        paddr_t          paddr,
        int              prot) {

    addr_t addr                 = vaddr;
    addr_space_t *addr_space    = &process->addr_space;

    for(size_t idx = 0; idx < length / PAGE_SIZE; ++idx) {
        /* TODO We should be able to optimize by not looking up the page table
         * for each entry. */
        if(!map_userspace_page(addr_space, addr, paddr, prot)) {
            return false;
        }

        addr += PAGE_SIZE;
        paddr += PAGE_SIZE;
    }

    return true;
}

/**
 * Unmap a page from virtual memory.
 *
 * There is no need to specify an address space for kernel mappings since
 * kernel mappings are global.
 *
 * This function does not perform any page table deallocation.
 *
 * @param addr_space address space from which to unmap, NULL for kernel mappings
 * @param addr address of page to unmap
 */
static void unmap_page(addr_space_t *addr_space, void *addr) {
    /** ASSERTION: addr is aligned on a page boundary */
    assert( page_offset_of(addr) == 0 );
    
    pte_t *pte = lookup_page_table_entry(addr_space, addr, false, NULL);
    
    if(pte != NULL) {
        clear_pte(pte);

        invalidate_mapping(addr_space, addr, false);
    }
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
void machine_unmap_kernel_page(void *addr) {
    assert(is_kernel_pointer(addr));
    unmap_page(NULL, addr);
}

/**
 * Clone mappings in range from one process' address space to another.
 *
 * After the operation, the range in the destination address space maps the same
 * memory as the range in the source address space. Any portion of the range
 * that does not point to memory in the source address space is unmapped in the
 * destination address space. The permissions of the new mappings are specified
 * by the prot argument.
 *
 * @param dest_process destination process
 * @param src_process source process
 * @param dest_addr start of range in destination address space
 * @param src_addr start of range in source address space
 * @param length length of mapping range
 * @param prot protections flags
 */
bool machine_clone_userspace_mapping(
        process_t   *dest_process,
        addr_t       dest_addr,
        process_t   *src_process,
        addr_t       src_addr,
        size_t       length,
        int          prot) {
    
    addr_space_t *src_addr_space    = &src_process->addr_space;
    addr_space_t *dest_addr_space   = &dest_process->addr_space;

    for(size_t idx = 0; idx < length / PAGE_SIZE; ++idx) {
        /* TODO We should be able to optimize by not looking up the page table
         * for each entry, both for source and destination. */
        pte_t *src_pte = lookup_page_table_entry(src_addr_space, src_addr, false, NULL);

        if(src_pte == NULL || !pte_is_present(src_pte)) {
            unmap_page(dest_addr_space, dest_addr);
        }
        else {
            paddr_t paddr = get_pte_paddr(src_pte);

            if(!map_userspace_page(dest_addr_space, dest_addr, paddr, prot)) {
                return false;
            }
        }

        src_addr += PAGE_SIZE;
        dest_addr += PAGE_SIZE;
    }

    return true;
}

/**
 * Look up the physical address of an existing kernel-mapped page frame.
 *
 * @param addr virtual address of kernel page
 * @return physical address of page frame
 */
paddr_t machine_lookup_kernel_paddr(const void *addr) {
    assert( is_kernel_pointer(addr) );

    pte_t *pte = lookup_page_table_entry(NULL, addr, false, NULL);

    assert(pte != NULL && pte_is_present(pte));

    return get_pte_paddr(pte);
}
