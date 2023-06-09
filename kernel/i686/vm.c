/*
 * Copyright (C) 2019-2023 Philippe Aubertin.
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

#include <jinue/shared/errno.h>
#include <kernel/i686/boot.h>
#include <kernel/i686/cpu_data.h>
#include <kernel/i686/memory.h>
#include <kernel/i686/vga.h>
#include <kernel/i686/vm.h>
#include <kernel/i686/vm_private.h>
#include <kernel/i686/x86.h>
#include <kernel/boot.h>
#include <kernel/elf.h>
#include <kernel/page_alloc.h>
#include <kernel/panic.h>
#include <kernel/util.h>
#include <kernel/vmalloc.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* This file contains memory management code that is independent of whether
 * Physical Address Extension (PAE) is enabled or not. The PAE code is in the
 * vm_pae.c file. Non-PAE code is in the vm_x86.c file.
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
 *
 */
static pte_t *get_pte_with_offset(pte_t *pte, unsigned int offset) {
    if(pgtable_format_pae) {
        return vm_pae_get_pte_with_offset(pte, offset);
    }
    else {
        return vm_x86_get_pte_with_offset(pte, offset);
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
 *
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
 *
 */
static unsigned int page_table_offset_of(void *addr) {
    if(pgtable_format_pae) {
        return vm_pae_page_table_offset_of(addr);
    }
    else {
        return vm_x86_page_table_offset_of(addr);
    }
}

/**
 * Get entry offset of specified virtual address within page directory
 *
 * @param addr virtual address
 * @return entry offset of address within page directory
 *
 */
static unsigned int page_directory_offset_of(void *addr) {
    if(pgtable_format_pae) {
        return vm_pae_page_directory_offset_of(addr);
    }
    else {
        return vm_x86_page_directory_offset_of(addr);
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
 *
 */
static void set_pte_flags(pte_t *pte, uint64_t flags) {
    if(pgtable_format_pae) {
        vm_pae_set_pte_flags(pte, flags);
    }
    else {
        vm_x86_set_pte_flags(pte, flags);
    }
}

/**
 * Copy a page table or page directory entry
 *
 * @param dest destination page table/directory entry
 * @param src source page table/directory entry
 *
 */
static void copy_pte(pte_t *dest, const pte_t *src) {
    if(pgtable_format_pae) {
        vm_pae_copy_pte(dest, src);
    }
    else {
        vm_x86_copy_pte(dest, src);
    }
}

/**
 * Copy an array of page table or page directory entries
 *
 * @param dest destination array
 * @param src source array
 * @param n number of entries to copy
 *
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
 *
 */
static void set_pte(pte_t *pte, user_paddr_t paddr, uint64_t flags) {
    if(pgtable_format_pae) {
        vm_pae_set_pte(pte, paddr, flags);
    }
    else {
        vm_x86_set_pte(pte, (uint32_t)paddr, flags);
    }
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
static void clear_pte(pte_t *pte) {
    if(pgtable_format_pae) {
        vm_pae_clear_pte(pte);
    }
    else {
        vm_x86_clear_pte(pte);
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
 *
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
 *
 */
static user_paddr_t get_pte_paddr(const pte_t *pte) {
    if(pgtable_format_pae) {
        return vm_pae_get_pte_paddr(pte);
    }
    else {
        return vm_x86_get_pte_paddr(pte);
    }
}

/**
 * Initialize virtual memory management to not use PAE
 *
 * During initialization, the kernel either calls this function or calls
 * vm_pae_enable() to enable PAE.
 *
 */
void vm_set_no_pae(void) {
    pgtable_format_pae      = false;
    entries_per_page_table  = VM_X86_PAGE_TABLE_PTES;
}

/**
 * Initialize consecutive page table entries to map consecutive page frames
 *
 * @param page_table first page table entry
 * @param start_paddr start physical address
 * @param flags page table entry flags
 * @param num_entries number of entries to initialize
 * @return first page table entry after affected ones
 *
 */
pte_t *vm_initialize_page_table_linear(
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
 * @param boot_info boot information structure
 *
 */
void vm_write_protect_kernel_image(const boot_info_t *boot_info) {
    size_t image_size = (char *)boot_info->image_top - (char *)boot_info->image_start;
    size_t image_pages = image_size / PAGE_SIZE;

    for(int idx = 0; idx < image_pages; ++idx) {
        set_pte_flags(
                get_pte_with_offset(boot_info->page_table_16mb, idx),
                X86_PTE_PRESENT); /* read only */
    }

    set_cr3((uintptr_t)boot_info->page_directory);
}

static void initialize_initial_page_tables(
        pte_t               *page_tables,
        Elf32_Ehdr          *kernel_elf,
        const boot_info_t   *boot_info) {

    size_t image_size  = (char *)boot_info->image_top - (char *)boot_info->image_start;
    size_t image_pages = image_size / PAGE_SIZE;

    /* map kernel image read only, not executable */
    pte_t *next_pte_after_image = vm_initialize_page_table_linear(
            page_tables,
            MEMORY_ADDR_16MB,
            X86_PTE_GLOBAL | X86_PTE_NX,
            image_pages);

    /* map rest of region read/write */
    vm_initialize_page_table_linear(
            next_pte_after_image,
            MEMORY_ADDR_16MB + image_size,
            X86_PTE_GLOBAL | X86_PTE_READ_WRITE | X86_PTE_NX,
            BOOT_SIZE_AT_16MB / PAGE_SIZE - image_pages);

    /* make kernel code segment executable */
    const Elf32_Phdr *phdr = elf_executable_program_header(kernel_elf);

    if(phdr == NULL) {
        panic("could not find kernel executable segment");
    }

    size_t code_offset = ((uintptr_t)phdr->p_vaddr - KLIMIT) / PAGE_SIZE;

    vm_initialize_page_table_linear(
            vm_pae_get_pte_with_offset(page_tables, code_offset),
            phdr->p_vaddr + MEMORY_ADDR_16MB - KLIMIT,
            X86_PTE_GLOBAL,
            phdr->p_memsz / PAGE_SIZE);

    /* map kernel data segment */
    size_t data_offset = ((uintptr_t)boot_info->data_start - KLIMIT) / PAGE_SIZE;

    vm_initialize_page_table_linear(
            vm_pae_get_pte_with_offset(page_tables, data_offset),
            boot_info->data_physaddr + MEMORY_ADDR_16MB - MEMORY_ADDR_1MB,
            X86_PTE_GLOBAL | X86_PTE_READ_WRITE | X86_PTE_NX,
            boot_info->data_size / PAGE_SIZE);
}

static void initialize_initial_page_directories(
        pte_t   *page_directories,
        pte_t   *page_tables,
        int      num_page_tables) {

    int offset = page_directory_offset_of((void *)KLIMIT);

    vm_initialize_page_table_linear(
            get_pte_with_offset(page_directories, offset),
            (uintptr_t)page_tables,
            X86_PTE_READ_WRITE,
            num_page_tables);
}

addr_space_t *vm_create_initial_addr_space(
        Elf32_Ehdr          *kernel_elf,
        boot_alloc_t        *boot_alloc,
        const boot_info_t   *boot_info) {

    /* Pre-allocate all the kernel page tables. */
    int num_pages           = (ADDR_4GB - KLIMIT) / PAGE_SIZE;
    int num_page_tables     = num_pages / entries_per_page_table;

    /* The number of entries in a pages table (page_table_entries) is also the
     * number of entries in a page directory. */
    int num_page_dirs       = ALIGN_END(num_page_tables, entries_per_page_table) / entries_per_page_table;

    /* allocate tables */
    pte_t *page_tables      = boot_page_alloc_n(boot_alloc, num_page_tables);
    pte_t *page_directories = boot_page_alloc_n(boot_alloc, num_page_dirs);

    initialize_initial_page_tables(page_tables, kernel_elf, boot_info);
    initialize_initial_page_directories(page_directories, page_tables, num_page_tables);

    kernel_page_tables      = (pte_t *)PHYS_TO_VIRT_AT_16MB(page_tables);

    if(pgtable_format_pae) {
        vm_pae_create_initial_addr_space(
                &initial_addr_space,
                page_directories,
                boot_alloc);
    }
    else {
        vm_x86_create_initial_addr_space(&initial_addr_space, page_directories);
    }

    return &initial_addr_space;
}

static pte_t *clone_first_kernel_page_directory(void) {
    pte_t *pd_template;

    if(pgtable_format_pae) {
        pd_template = vm_pae_lookup_page_directory(
                &initial_addr_space,
                (void *)KLIMIT,
                false,
                NULL);
    }
    else {
        pd_template = vm_x86_lookup_page_directory(&initial_addr_space);
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

addr_space_t *vm_create_addr_space(addr_space_t *addr_space) {
    pte_t *page_directory = clone_first_kernel_page_directory();

    if(page_directory == NULL) {
        return NULL;
    }

    if(pgtable_format_pae) {
        addr_space_t *retval = vm_pae_create_addr_space(addr_space, page_directory);

        if(retval == NULL) {
            free_first_kernel_page_directory(page_directory);
        }

        return retval;
    }
    else {
        return vm_x86_create_addr_space(addr_space, page_directory);
    }
}

void vm_destroy_page_directory(void *page_directory, unsigned int last_index) {
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

void vm_destroy_addr_space(addr_space_t *addr_space) {
    /** ASSERTION: address space must not be NULL */
    assert(addr_space != NULL);

    /** ASSERTION: the initial address space should not be destroyed */
    assert(addr_space != &initial_addr_space);

    /** ASSERTION: the current address space should not be destroyed */
    assert( addr_space != get_current_addr_space() );

    if(pgtable_format_pae) {
        vm_pae_destroy_addr_space(addr_space);
    }
    else {
        vm_x86_destroy_addr_space(addr_space);
    }
}

void vm_switch_addr_space(addr_space_t *addr_space, cpu_data_t *cpu_data) {
    set_cr3(addr_space->cr3);
    cpu_data->current_addr_space = addr_space;
}

void vm_boot_map(void *addr, uint32_t paddr, int num_entries) {
    int offset = (uintptr_t)((char *)addr - KLIMIT) / PAGE_SIZE;

    vm_initialize_page_table_linear(
            get_pte_with_offset(
                    (pte_t *)PTR_TO_PHYS_ADDR_AT_16MB(kernel_page_tables),
                    offset),
            paddr,
            X86_PTE_READ_WRITE,
            num_entries);
}

static pte_t *vm_lookup_page_table(
        addr_space_t    *addr_space,
        void            *addr,
        bool             create_as_needed,
        bool            *reload_cr3) {

    pte_t *page_directory;

    /** ASSERTION: addr_space cannot be NULL for non-global mappings */
    assert(addr_space != NULL);

    if(pgtable_format_pae) {
        page_directory = vm_pae_lookup_page_directory(
                addr_space,
                addr,
                create_as_needed,
                reload_cr3);
    }
    else {
        page_directory = vm_x86_lookup_page_directory(addr_space);
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

        set_pte(pde, vm_lookup_kernel_paddr(page_table), flags);
    }

    return page_table;
}

/**
    Lookup a page table entry for a specified address and address space.

    If the create_as_needed argument is true, new page tables are allocated as
    needed. Otherwise, NULL is returned if there is currently no page table for
    the specified address and address space.

    @param addr_space address space in which the address is looked up.
    @param addr address to look up
    @param create_as_needed whether a page table is allocated if it does not exist
*/
static pte_t *vm_lookup_page_table_entry(
        addr_space_t    *addr_space,
        void            *addr,
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

    pte_t *page_table = vm_lookup_page_table(
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
 *
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
            invalidate_tlb(addr);
        }
    }
}

/**
 * Translate architecture-independent protection flags to architecture-dependent flags
 *
 * Argument should be either VM_MAP_NONE (no access) or any combination of
 * VM_MAP_READ, VM_MAP_WRITE and/or VM_MAP_EXEC.
 *
 * Return value is a combination of architecture-dependent flags appropriate to
 * be set directly in a page table entry (i.e. the X86_PTE_... constants).
 *
 * @param pte flags architecture-independent protection flags
 * @return architecture-dependent flags
 *
 */
static uint64_t map_page_access_flags(int flags) {
    const int rwe_mask = VM_MAP_READ | VM_MAP_WRITE | VM_MAP_EXEC;

    if(! (flags & rwe_mask)) {
        return X86_PTE_PROT_NONE;
    }

    int mapped_flags = X86_PTE_PRESENT;

    if(flags & VM_MAP_WRITE) {
        mapped_flags |= X86_PTE_READ_WRITE;
    }

    if(! (flags & VM_MAP_EXEC)) {
        mapped_flags |= X86_PTE_NX;
    }

    return mapped_flags;
}

/**
 * Map a page frame (physical page) to a virtual memory page.
 *
 * This function is intended to be called by vm_map_userspace and vm_map_kernel().
 * Either of these functions should be called elsewhere instead of calling this
 * function directly.
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
 * @param flags flags used for mapping (see VM_FLAG_x constants in vm.h)
 * @return true on success, false on page table allocation error
 *
 */
static bool vm_map(
        addr_space_t    *addr_space,
        void            *vaddr,
        user_paddr_t     paddr,
        uint64_t         flags) {

    /** ASSERTION: we assume vaddr is aligned on a page boundary */
    assert( page_offset_of(vaddr) == 0 );
    
    bool reload_cr3 = false;
    pte_t *pte = vm_lookup_page_table_entry(addr_space, vaddr, true, &reload_cr3);
    
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
 * @param flags flags used for the mapping (see VM_FLAG_x constants in vm.h)
 *
 */
void vm_map_kernel(void *vaddr, kern_paddr_t paddr, int flags) {
    assert(is_kernel_pointer(vaddr));

    vm_map(NULL, vaddr, paddr, map_page_access_flags(flags) | X86_PTE_GLOBAL);
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
 * @param flags flags used for the mapping (see VM_FLAG_x constants in vm.h)
 * @return true on success, false on page table allocation error
 *
 */
bool vm_map_userspace(
        addr_space_t    *addr_space,
        void            *vaddr,
        user_paddr_t     paddr,
        int              flags) {

    assert(is_userspace_pointer(vaddr));

    return vm_map(addr_space, vaddr, paddr, map_page_access_flags(flags) | X86_PTE_USER);
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
 *
 */
static void vm_unmap(addr_space_t *addr_space, void *addr) {
    /** ASSERTION: addr is aligned on a page boundary */
    assert( page_offset_of(addr) == 0 );
    
    pte_t *pte = vm_lookup_page_table_entry(addr_space, addr, false, NULL);
    
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
 *
 */
void vm_unmap_kernel(void *addr) {
    assert(is_kernel_pointer(addr));
    vm_unmap(NULL, addr);
}

/**
 * Unmap a user space page from virtual memory.
 *
 * This function does not perform any page table deallocation.
 *
 * @param addr_space address space from which to unmap
 * @param addr address of page to unmap
 *
 */
void vm_unmap_userspace(addr_space_t *addr_space, void *addr) {
    assert(is_userspace_pointer(addr));
    vm_unmap(addr_space, addr);
}

/**
 * Change the flags for the existing mapping of a single page.
 *
 * An example use case is to convert a read/write page mapping to a read-only
 * mapping or vice versa.
 *
 * @param addr_space address space of the existing mapping
 * @param addr address of page
 * @param flags new flags for mapping
 *
 */
void vm_change_flags(addr_space_t *addr_space, addr_t addr, int flags) {
    pte_t *pte = vm_lookup_page_table_entry(addr_space, addr, false, NULL);
    
    assert(pte != NULL && pte_is_present(pte));
    
    set_pte_flags(pte, map_page_access_flags(flags));
    
    invalidate_mapping(addr_space, addr, false);
}

/**
 * Look up the physical address of an existing kernel-mapped page frame.
 *
 * @param addr virtual address of kernel page
 * @return physical address of page frame
 *
 */
kern_paddr_t vm_lookup_kernel_paddr(void *addr) {
    assert( is_kernel_pointer(addr));

    pte_t *pte = vm_lookup_page_table_entry(NULL, addr, false, NULL);

    assert(pte != NULL && pte_is_present(pte));

    return (kern_paddr_t)get_pte_paddr(pte);
}

int vm_mmap_syscall(int process_fd, const jinue_mmap_args_t *args) {
    return -JINUE_ENOSYS;
}
