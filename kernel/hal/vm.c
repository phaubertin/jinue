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

#include <hal/boot.h>
#include <hal/cpu_data.h>
#include <hal/memory.h>
#include <hal/vga.h>
#include <hal/vm.h>
#include <hal/vm_private.h>
#include <hal/x86.h>
#include <assert.h>
#include <boot.h>
#include <page_alloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <util.h>
#include <vmalloc.h>


pte_t *kernel_page_tables;

pte_t *kernel_page_directories;

addr_space_t initial_addr_space;

size_t page_table_entries;

bool pgtable_format_pae;

static pte_t *get_pte_with_offset(pte_t *pte, unsigned int offset) {
    if(pgtable_format_pae) {
        return vm_pae_get_pte_with_offset(pte, offset);
    }
    else {
        return vm_x86_get_pte_with_offset(pte, offset);
    }
}

static inline const pte_t *get_pte_with_offset_const(
        const pte_t     *pte,
        unsigned int     offset) {

    return get_pte_with_offset((pte_t *)pte, offset);
}

static unsigned int page_table_offset_of(void *addr) {
    if(pgtable_format_pae) {
        return vm_pae_page_table_offset_of(addr);
    }
    else {
        return vm_x86_page_table_offset_of(addr);
    }
}

static unsigned int page_directory_offset_of(void *addr) {
    if(pgtable_format_pae) {
        return vm_pae_page_directory_offset_of(addr);
    }
    else {
        return vm_x86_page_directory_offset_of(addr);
    }
}

static void set_pte_flags(pte_t *pte, int flags) {
    if(pgtable_format_pae) {
        vm_pae_set_pte_flags(pte, flags);
    }
    else {
        vm_x86_set_pte_flags(pte, flags);
    }
}

static int get_pte_flags(const pte_t *pte) {
    if(pgtable_format_pae) {
        return vm_pae_get_pte_flags(pte);
    }
    else {
        return vm_x86_get_pte_flags(pte);
    }
}

static void copy_pte(pte_t *dest, const pte_t *src) {
    if(pgtable_format_pae) {
        vm_pae_copy_pte(dest, src);
    }
    else {
        vm_x86_copy_pte(dest, src);
    }
}

void copy_ptes(pte_t *dest, const pte_t *src, int n) {
    for(int idx = 0; idx < n; ++idx) {
        copy_pte(
                get_pte_with_offset(dest, idx),
                get_pte_with_offset_const(src, idx));
    }
}

static void set_pte(pte_t *pte, user_paddr_t paddr, int flags) {
    if(pgtable_format_pae) {
        vm_pae_set_pte(pte, paddr, flags);
    }
    else {
        vm_x86_set_pte(pte, (uint32_t)paddr, flags);
    }
}

static void clear_pte(pte_t *pte) {
    if(pgtable_format_pae) {
        vm_pae_clear_pte(pte);
    }
    else {
        vm_x86_clear_pte(pte);
    }
}

void clear_ptes(pte_t *pte, int n) {
    for(int idx = 0; idx < n; ++idx) {
        clear_pte(get_pte_with_offset(pte, idx));
    }
}

static user_paddr_t get_pte_paddr(const pte_t *pte) {
    if(pgtable_format_pae) {
        return vm_pae_get_pte_paddr(pte);
    }
    else {
        return vm_x86_get_pte_paddr(pte);
    }
}

void vm_set_no_pae(void) {
    pgtable_format_pae = false;
    page_table_entries = VM_X86_PAGE_TABLE_PTES;
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
void vm_initialize_page_table_linear(
        pte_t       *page_table,
        uint64_t     start_paddr,
        int          flags,
        int          num_entries) {

    uint64_t paddr  = start_paddr;

    for(int idx = 0; idx < num_entries; ++idx) {
        set_pte(
                get_pte_with_offset(page_table, idx),
                paddr,
                flags | VM_FLAG_PRESENT);

        paddr += PAGE_SIZE;
    }
}

addr_space_t *vm_create_initial_addr_space(boot_alloc_t *boot_alloc) {
    /* Pre-allocate all the kernel page tables. */
    int num_pages       = (ADDR_4GB - KLIMIT) / PAGE_SIZE;
    int num_page_tables = num_pages / page_table_entries;
    pte_t *page_tables  = boot_page_alloc_n(boot_alloc, num_page_tables);
    kernel_page_tables  = (pte_t *)PHYS_TO_VIRT_AT_16MB(page_tables);

    /* Initialize the first few page tables to map BOOT_SIZE_AT_16MB starting
     * at 0x1000000 (i.e. at 16MB). */
    vm_initialize_page_table_linear(
            page_tables,
            MEMORY_ADDR_16MB,
            VM_FLAG_READ_WRITE | VM_FLAG_KERNEL,
            BOOT_SIZE_AT_16MB / PAGE_SIZE);

    /* The number of entries in a pages table (page_table_entries) is also the
     * number of entries in a page directory. */
    int num_page_dirs       = ALIGN_END(num_page_tables, page_table_entries) / page_table_entries;
    int offset              = page_directory_offset_of((void *)KLIMIT);
    pte_t *page_directories = boot_page_alloc_n(boot_alloc, num_page_dirs);
    kernel_page_directories = (pte_t *)PHYS_TO_VIRT_AT_16MB(page_directories);

    vm_initialize_page_table_linear(
            get_pte_with_offset(page_directories, offset),
            (uintptr_t)page_tables,
            VM_FLAG_READ_WRITE,
            num_page_tables);

    if(pgtable_format_pae) {
        return vm_pae_create_initial_addr_space(page_directories, boot_alloc);
    }
    else {
        return vm_x86_create_initial_addr_space(page_directories);
    }
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
                page_table_entries - klimit_offset);
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

void vm_destroy_page_directory(
        kern_paddr_t         pgdir_paddr,
        unsigned int         from_index,
        unsigned int         to_index) {

    pte_t *page_directory = (pte_t *)vmalloc();
    vm_map_kernel(page_directory, pgdir_paddr, VM_FLAG_READ_WRITE);

    /* be careful not to free the kernel page tables */
    for(unsigned int idx = from_index; idx < to_index; ++idx) {
        pte_t *pte = get_pte_with_offset(page_directory, idx);

        if(get_pte_flags(pte) & VM_FLAG_PRESENT) {
            /* TODO fix this */
            //pffree( get_pte_paddr(pte) );
        }
    }

    /* TODO fix this */
    vm_unmap_kernel(page_directory);
    //pffree(pgdir_paddr);
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
            VM_FLAG_READ_WRITE,
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

    if(get_pte_flags(pde) & VM_FLAG_PRESENT) {
        return memory_lookup_page(get_pte_paddr(pde));
    }

    if(! create_as_needed) {
        return NULL;
    }

    pte_t *page_table = page_alloc();

    if(page_table != NULL) {
        int access_flags;

        clear_page(page_table);

        if(is_user_pointer(addr)) {
            access_flags = VM_FLAG_USER;
        }
        else {
            /* Do not use VM_FLAG_KERNEL here. VM_FLAG_KERNEL is intended
             * for page table entries, not page directory entries. */
            access_flags = 0;
        }

        set_pte(
                pde,
                vm_lookup_kernel_paddr(page_table),
                access_flags | VM_FLAG_READ_WRITE | VM_FLAG_PRESENT);
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

static void invalidate_mapping(
        addr_space_t    *addr_space,
        void            *vaddr,
        bool             reload_cr3) {

    if(reload_cr3) {
        assert(addr_space != NULL);

        uint32_t cr3 = get_cr3();

        if(cr3 == addr_space->cr3) {
            set_cr3(cr3);
        }
    }
    else {
        if(addr_space == NULL || addr_space->cr3 == get_cr3()) {
            invalidate_tlb(vaddr);
        }
    }
}

/** 
    Map a page frame (physical page) to a virtual memory page.
    @param addr_space address space in which to map, can be NULL for global mappings (vaddr >= KLIMIT)
    @param vaddr virtual address of mapping
    @param paddr address of page frame
    @param flags flags used for mapping (see VM_FLAG_x constants in vm.h)
    @return true on success, false on page table allocation error
*/
static bool vm_map(
        addr_space_t    *addr_space,
        void            *vaddr,
        user_paddr_t     paddr,
        int              flags) {

    /** ASSERTION: we assume vaddr is aligned on a page boundary */
    assert( page_offset_of(vaddr) == 0 );
    
    bool reload_cr3 = false;
    pte_t *pte = vm_lookup_page_table_entry(addr_space, vaddr, true, &reload_cr3);
    
    /* kernel page tables are pre-allocated so this should always succeed for
     * kernel mappings. */
    assert(pte != NULL || is_user_pointer(vaddr));
    
    if(pte == NULL) {
        return false;
    }

    set_pte(pte, paddr, flags | VM_FLAG_PRESENT);

    invalidate_mapping(addr_space, vaddr, reload_cr3);

    return true;
}

void vm_map_kernel(void *vaddr, kern_paddr_t paddr, int flags) {
    assert(is_kernel_pointer(vaddr));

    vm_map(NULL, vaddr, paddr, flags | VM_FLAG_KERNEL);
}

bool vm_map_userspace(
        addr_space_t    *addr_space,
        void            *vaddr,
        user_paddr_t     paddr,
        int              flags) {

    assert(is_user_pointer(vaddr));

    return vm_map(addr_space, vaddr, paddr, flags | VM_FLAG_USER);
}

/**
    Unmap a page from virtual memory.
    @param addr_space address space from which to unmap, can be NULL for global mappings (addr >= KLIMIT)
    @param addr address of page to unmap
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

void vm_unmap_kernel(void *addr) {
    vm_unmap(NULL, addr);
}

void vm_unmap_userspace(addr_space_t *addr_space, void *addr) {
    vm_unmap(addr_space, addr);
}

void vm_change_flags(addr_space_t *addr_space, addr_t addr, int flags) {
    pte_t *pte = vm_lookup_page_table_entry(addr_space, addr, false, NULL);
    
    /** ASSERTION: there is a page table entry marked present for this address */
    assert(pte != NULL && (get_pte_flags(pte) & VM_FLAG_PRESENT));
    
    /* perform the flags change */
    set_pte_flags(pte, flags | VM_FLAG_PRESENT);
    
    invalidate_mapping(addr_space, addr, false);
}

kern_paddr_t vm_lookup_kernel_paddr(void *addr) {
    /** ASSERTION: addr is a kernel virtual address */
    assert( is_kernel_pointer(addr));

    pte_t *pte = vm_lookup_page_table_entry(NULL, addr, false, NULL);

    /** ASSERTION: there is a page table entry marked present for this address */
    assert(pte != NULL && (get_pte_flags(pte) & VM_FLAG_PRESENT));

    return (kern_paddr_t)get_pte_paddr(pte);
}
