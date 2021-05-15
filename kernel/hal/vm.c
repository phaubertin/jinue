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
#include <hal/vga.h>
#include <hal/vm.h>
#include <hal/vm_private.h>
#include <hal/x86.h>
#include <assert.h>
#include <boot.h>
#include <page_alloc.h>
#include <pfalloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <vmalloc.h>


pte_t *global_page_tables;

addr_space_t initial_addr_space;

size_t page_table_entries;

bool pgtable_format_pae;

void vm_set_no_pae(void) {
    pgtable_format_pae = false;
    page_table_entries = VM_X86_PAGE_TABLE_PTES;
}

static pte_t *get_pte_with_offset(pte_t *pte, unsigned int offset) {
    if(pgtable_format_pae) {
        return vm_pae_get_pte_with_offset(pte, offset);
    }
    else {
        return vm_x86_get_pte_with_offset(pte, offset);
    }
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

static user_paddr_t get_pte_paddr(const pte_t *pte) {
    if(pgtable_format_pae) {
        return vm_pae_get_pte_paddr(pte);
    }
    else {
        return vm_x86_get_pte_paddr(pte);
    }
}

/** 
    Lookup a page table entry for a specified address and address space.
    
    If the create_as_needed argument is true, new page tables are allocated as
    needed. Otherwise, NULL is returned if there is currently no page table for
    the specified address and address space.
    
    If a non-NULL value is returned, it is the caller's responsibility to call
    vm_free_page_table_entry() when they are done with it.
    
    Important note: This function temporarily maps the page table that contains
    the returned page table entry and allocates a page of virtual memory for
    that purpose. The caller must call the vm_free_page_table_entry() function
    to free that mapping when it is done with the page table entry.
    
    @param addr_space address space in which the address is looked up.
    @param addr address to look up
    @param create_as_need whether a page table is allocated if it does not exist
*/
static pte_t *vm_lookup_page_table_entry(addr_space_t *addr_space, void *addr, bool create_as_needed) {
    /** ASSERTION: we assume addr is aligned on a page boundary */
    assert( page_offset_of(addr) == 0 );
    
    if(is_fast_map_pointer(addr)) {
        /* Fast path for global allocations by the kernel:
         *  - The page tables for the region just above KLIMIT are pre-allocated
         *    during the creation of the address space, so there is no need to
         *    check if they are allocated or to allocate them;
         *  - The page tables are mapped contiguously at a known location
         *    during initialization, so no need to find and map them, and they
         *    can be indexed as a single big page table;
         *  - The mappings for this region are global, so we don't care
         *    about the specified address space.  */
        return get_pte_with_offset(global_page_tables, page_number_of((uintptr_t)addr - KLIMIT));
    }
    else {
        pte_t *page_directory;
        pte_t *page_table;
        pte_t *pte;
    
        /** ASSERTION: addr_space cannot be NULL for non-global mappings */
        assert(addr_space != NULL);
        
        /* map page directory temporarily */
        if(pgtable_format_pae) {
            page_directory = vm_pae_lookup_page_directory(addr_space, addr, create_as_needed);
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
            /* map page table */
            page_table = (pte_t *)vmalloc();
            
            vm_map_kernel((addr_t)page_table, get_pte_paddr(pde), VM_FLAG_READ_WRITE);
            
            /* get page table entry */
            pte = get_pte_with_offset(page_table, page_table_offset_of(addr));
        }
        else {
            if(create_as_needed) {
                kern_paddr_t pgtable_paddr;
                
                /* allocate a new page table and map it */
                /* TODO both of these allocations can fail. */
                page_table      = (pte_t *)vmalloc();
                pgtable_paddr   = pfalloc();
            
                vm_map_kernel((addr_t)page_table, pgtable_paddr, VM_FLAG_READ_WRITE);
                
                /* zero content of page table */
                memset(page_table, 0, PAGE_SIZE);
                
                /* link page table in page directory */
                int access_flags;
                
                if(is_user_pointer(addr)) {
                    access_flags = VM_FLAG_USER;
                }
                else {
                    /* Do not use VM_FLAG_KERNEL here. VM_FLAG_KERNEL is intended
                     * for page table entries, not page directory entries. */
                    access_flags = 0;
                }
                
                set_pte(pde, pgtable_paddr, access_flags | VM_FLAG_READ_WRITE | VM_FLAG_PRESENT);
                
                /* get page table entry */
                pte = get_pte_with_offset(page_table, page_table_offset_of(addr));
            }
            else {
                /* address has no mapping, return NULL */
                pte = NULL;
            }
        }
        
        /* unmap page directory and free the (virtual) page allocated to it */
        vm_unmap_kernel((addr_t)page_directory);
        vmfree((addr_t)page_directory);
        
        return pte;
    }
}

/** 
    Free temporary page table mapping.
    
    This function must be called whenever the caller is done with a page table
    entry returned by vm_lookup_page_table_entry().
    
    @param addr the address that was passed to vm_lookup_page_table_entry()
    @param pte pointer to page table entry returned by  vm_lookup_page_table_entry()
*/
static void vm_free_page_table_entry(void *addr, pte_t *pte) {
    /* The address range where is_fast_map_pointer(addr) is true is a range where
     * all mappings are global and where the page tables are permanently mapped in
     * all address spaces.
     * 
     * The page table need not and must not be freed for addresses in that range. */
    if(! is_fast_map_pointer(addr)) {
        void *page_table = (void *)((uintptr_t)pte & ~PAGE_MASK);
        vm_unmap_kernel(page_table);
        vmfree(page_table);
    }
}

/** 
    Map a page frame (physical page) to a virtual memory page.
    @param addr_space address space in which to map, can be NULL for global mappings (vaddr >= KLIMIT)
    @param vaddr virtual address of mapping
    @param paddr address of page frame
    @param flags flags used for mapping (see VM_FLAG_x constants in vm.h)
*/
static void vm_map(addr_space_t *addr_space, addr_t vaddr, user_paddr_t paddr, int flags) {
    /** ASSERTION: we assume vaddr is aligned on a page boundary */
    assert( page_offset_of(vaddr) == 0 );
    
    pte_t *pte = vm_lookup_page_table_entry(addr_space, vaddr, true);
    
    /** ASSERTION: vm_lookup_page_table_entry() should not return NULL since its create_as_needed argument is true */
    assert(pte != NULL);
    
    set_pte(pte, paddr, flags | VM_FLAG_PRESENT);
    
    vm_free_page_table_entry(vaddr, pte);
        
    /* invalidate TLB entry for newly mapped page */
    invalidate_tlb(vaddr);
}

/**
    Unmap a page from virtual memory.
    @param addr_space address space from which to unmap, can be NULL for global mappings (addr >= KLIMIT)
    @param addr address of page to unmap
*/
static void vm_unmap(addr_space_t *addr_space, addr_t addr) {
    /** ASSERTION: we assume addr is aligned on a page boundary */
    assert( page_offset_of(addr) == 0 );
    
    pte_t *pte = vm_lookup_page_table_entry(addr_space, addr, false);
    
    if(pte != NULL) {
        clear_pte(pte);
    
        vm_free_page_table_entry(addr, pte);
        
        /* invalidate TLB entry for newly mapped page */
        invalidate_tlb(addr);
    }
}

void vm_map_kernel(addr_t vaddr, kern_paddr_t paddr, int flags) {
    vm_map(NULL, vaddr, paddr, flags | VM_FLAG_KERNEL);
}

void vm_map_user(addr_space_t *addr_space, addr_t vaddr, user_paddr_t paddr, int flags) {
    vm_map(addr_space, vaddr, paddr, flags | VM_FLAG_USER);
}

void vm_unmap_kernel(addr_t addr) {
    vm_unmap(NULL, addr);
}

void vm_unmap_user(addr_space_t *addr_space, addr_t addr) {
    vm_unmap(addr_space, addr);
}

kern_paddr_t vm_lookup_kernel_paddr(addr_t addr) {
    pte_t *pte = vm_lookup_page_table_entry(NULL, addr, false);
    
    /** ASSERTION: there is a page table entry marked present for this address */
    assert(pte != NULL && (get_pte_flags(pte) & VM_FLAG_PRESENT));
    
    kern_paddr_t paddr = (kern_paddr_t)get_pte_paddr(pte);
    
    vm_free_page_table_entry(addr, pte);
    
    return paddr;
}

void vm_change_flags(addr_space_t *addr_space, addr_t addr, int flags) {
    pte_t *pte = vm_lookup_page_table_entry(addr_space, addr, false);
    
    /** ASSERTION: there is a page table entry marked present for this address */
    assert(pte != NULL && (get_pte_flags(pte) & VM_FLAG_PRESENT));
    
    /* perform the flags change */
    set_pte_flags(pte, flags | VM_FLAG_PRESENT);
    
    vm_free_page_table_entry(addr, pte);
    
    /* invalidate TLB entry for the affected page */
    invalidate_tlb(addr);
}

void vm_boot_map(void *addr, uint32_t paddr, int num_entries) {
    int offset = (uintptr_t)((char *)addr - KLIMIT) / PAGE_SIZE;

    vm_initialize_page_table_linear(
            get_pte_with_offset(
                    (pte_t *)PTR_TO_PHYS_ADDR_AT_16MB(global_page_tables),
                    offset),
            paddr,
            VM_FLAG_READ_WRITE,
            num_entries);
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

    pte_t *pte      = page_table;
    uint64_t paddr  = start_paddr;

    for(int idx = 0; idx < num_entries; ++idx) {
        set_pte(pte, paddr, flags | VM_FLAG_PRESENT);

        paddr += PAGE_SIZE;
        pte = get_pte_with_offset(pte, 1);
    }
}

kern_paddr_t vm_clone_page_directory(kern_paddr_t template_paddr, unsigned int start_index) {
    unsigned int     idx;
    
    /* Allocate new page directory.
     *
     * TODO handle allocation failure */
    pte_t *page_directory = (pte_t *)page_alloc();

    /* map page directory template */
    pte_t *template = (pte_t *)vmalloc();
    vm_map_kernel((addr_t)template, template_paddr, VM_FLAG_READ_WRITE);

    /* clear all entries below index start_index */
    for(idx = 0; idx < start_index; ++idx) {
        clear_pte( get_pte_with_offset(page_directory, idx) );
    }

    /* copy entries from template for indexes start_index and above */
    for(idx = start_index; idx < page_table_entries; ++idx) {
        copy_pte(
            get_pte_with_offset(page_directory, idx),
            get_pte_with_offset(template, idx)
        );
    }
    
    vm_unmap_kernel((addr_t)page_directory);
    vm_unmap_kernel((addr_t)template);
    
    return vm_lookup_kernel_paddr((addr_t)page_directory);
}

addr_space_t *vm_create_addr_space(addr_space_t *addr_space) {
    if(pgtable_format_pae) {
        return vm_pae_create_addr_space(addr_space);
    }
    else {
        return vm_x86_create_addr_space(addr_space);
    }
}

addr_space_t *vm_create_initial_addr_space(boot_alloc_t *boot_alloc) {
    /* Pre-allocate all the kernel page tables. */
    int num_pages       = (ADDR_4GB - KLIMIT) / PAGE_SIZE;
    int num_page_tables = num_pages / page_table_entries;
    pte_t *page_tables  = boot_page_alloc_n(boot_alloc, num_page_tables);
    global_page_tables  = (pte_t *)PHYS_TO_VIRT_AT_16MB(page_tables);

    /* Initialize the first few page tables to map BOOT_SIZE_AT_16MB starting
     * at 0x1000000 (i.e. at 16MB). */
    vm_initialize_page_table_linear(
            page_tables,
            MEMORY_ADDR_16MB,
            VM_FLAG_READ_WRITE,
            BOOT_SIZE_AT_16MB / PAGE_SIZE);

    int offset = page_directory_offset_of((void *)KLIMIT);
    pte_t *page_directory = boot_page_alloc(boot_alloc);

    vm_initialize_page_table_linear(
            get_pte_with_offset(page_directory, offset),
            (uintptr_t)page_tables,
            VM_FLAG_READ_WRITE,
            page_table_entries - offset);

    if(pgtable_format_pae) {
        return vm_pae_create_initial_addr_space(page_directory, boot_alloc);
    }
    else {
        return vm_x86_create_initial_addr_space(page_directory);
    }
}

void vm_destroy_page_directory(kern_paddr_t pgdir_paddr, unsigned int from_index, unsigned int to_index) {
    unsigned int idx;
   
    pte_t *page_directory = (pte_t *)vmalloc();
    vm_map_kernel((addr_t)page_directory, pgdir_paddr, VM_FLAG_READ_WRITE);
    
    /* be careful not to free the kernel page tables */
    for(idx = from_index; idx < to_index; ++idx) {
        pte_t *pte = get_pte_with_offset(page_directory, idx);
        
        if(get_pte_flags(pte) & VM_FLAG_PRESENT) {
            pffree( get_pte_paddr(pte) );
        }
    }
    
    vm_unmap_kernel((addr_t)page_directory);
    pffree(pgdir_paddr);
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
