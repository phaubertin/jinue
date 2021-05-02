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
#include <printk.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <vmalloc.h>


pte_t *global_page_tables;

addr_space_t initial_addr_space;

size_t page_table_entries;

static bool pgtable_format_pae;

void vm_boot_init(
        const boot_info_t   *boot_info,
        bool                 use_pae,
        cpu_data_t          *cpu_data,
        boot_alloc_t        *boot_alloc) {
    
    addr_t addr;

    if(use_pae) {
        vm_pae_boot_init();
    }
    else {
        vm_x86_boot_init();
    }
    
    pgtable_format_pae = use_pae;

    /* create initial address space */
    addr_space_t *addr_space = vm_create_initial_addr_space(use_pae, boot_alloc);
    
    /** below this point, it is no longer safe to call pfalloc_early() */
    boot_alloc->its_early = false;
    
    /* perform 1:1 mapping of kernel image and data */
    for(addr = (addr_t)boot_info->image_start; addr < boot_alloc->kernel_vm_top; addr += PAGE_SIZE) {
        vm_map_early(addr, EARLY_PTR_TO_PHYS_ADDR(addr), VM_FLAG_KERNEL | VM_FLAG_READ_WRITE);
    }
    
    /* map VGA text buffer in the new address space
     * 
     * This is a good place to do this because:
     * 
     * 1) It is our last chance to allocate a continuous region of virtual memory.
     *    Once the page allocator is initialized (see call to vm_alloc_init_allocator()
     *    below) and we start using vm_alloc() to allocate memory, pages can only
     *    be allocated one at a time.
     * 
     * 2) Doing this last makes things simpler because this is the only place where
     *    we have to allocate a continuous region of virtual memory but no physical
     *    memory to back it. To allocate it, we just have to increase kernel_vm_top,
     *    which represents the end of the virtual memory region that is used by the
     *    kernel. */
    addr_t vga_text_base;
    kern_paddr_t paddr = VGA_TEXT_VID_BASE;

    while(paddr < VGA_TEXT_VID_TOP) {
        /* Pages allocated by successive calls to vmalloc_boot() are guaranteed
         * to be contiguous. */
        addr = boot_vmalloc(boot_alloc);

        if(paddr == VGA_TEXT_VID_BASE) {
            /* First iteration */
            vga_text_base = addr;
        }

        vm_map_early(addr, paddr, VM_FLAG_KERNEL | VM_FLAG_READ_WRITE);
        paddr           += PAGE_SIZE;
    }

    /* remap VGA text buffer
     * 
     * Note: after the call to vga_set_base_addr() below until we switch to the
     * new address space, VGA output is not possible. Calling printk() will cause
     * a kernel panic due to a page fault (and the panic handler calls printk()). */
    printk("Remapping text video memory at 0x%x\n", vga_text_base);
    
    vga_set_base_addr(vga_text_base);

    /* switch to new address space */
    vm_switch_addr_space(addr_space, cpu_data);
}

void vm_boot_postinit(const boot_info_t *boot_info, boot_alloc_t *boot_alloc, bool use_pae) {
    /* initialize global page allocator (region starting at KLIMIT)
     * 
     * TODO Some work needs to be done in the page allocator to support allocating
     * up to the top of memory (i.e. 0x100000000, which cannot be represented on
     * 32 bits). In the mean time, we leave a 4MB (one block) gap. */
    vmalloc_init(
            (addr_t)KERNEL_IMAGE_END,
            (addr_t)0 - 4 * MB,
            (addr_t)KERNEL_PREALLOC_LIMIT,
            boot_alloc);

    /* create slab cache to allocate PDPTs
     * 
     * This must be done after the global page allocator has been initialized
     * because the slab allocator needs to allocate a slab to allocate the new
     * slab cache on the slab cache cache.
     * 
     * This must be done before the first time vm_create_addr_space() is called. */
    if(use_pae) {
        vm_pae_create_pdpt_cache(boot_alloc);
    }
}

static pte_t *get_pte_with_offset(pte_t *pte, unsigned int offset) {
    if(pgtable_format_pae) {
        return vm_pae_get_pte_with_offset(pte, offset);
    }
    else {
        return vm_x86_get_pte_with_offset(pte, offset);
    }
}

static unsigned int page_table_offset_of(addr_t addr) {
    if(pgtable_format_pae) {
        return vm_pae_page_table_offset_of(addr);
    }
    else {
        return vm_x86_page_table_offset_of(addr);
    }
}

static unsigned int page_directory_offset_of(addr_t addr) {
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

void vm_map_early(addr_t vaddr, kern_paddr_t paddr, int flags) {
    /** ASSERTION: we are within the mapping set up by the setup code */
    assert( is_early_pointer(vaddr) );
    
    /** ASSERTION: we assume vaddr is aligned on a page boundary */
    assert( page_offset_of(vaddr) == 0 );
    
    pte_t *pte = get_pte_with_offset(global_page_tables, page_number_of(vaddr - KLIMIT));
    set_pte(pte, paddr, flags | VM_FLAG_PRESENT);
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

void vm_init_initial_page_directory(
        pte_t           *page_directory,
        boot_alloc_t    *boot_alloc,
        unsigned int     start_index,
        unsigned int     end_index,
        bool             first_directory) {

    unsigned int idx, idy;

    /* Allocate page tables and initialize page directory entries. */
    for(idx = 0; idx < page_table_entries; ++idx) {
        if(idx < start_index || idx >= end_index) {
            /* Clear page directory entries for user space and non-preallocated
             * kernel page tables. */
            clear_pte( get_pte_with_offset(page_directory, idx) );
        }
        else {
            /* Allocate page tables for kernel data/code region.
             *
             * Note that the use of pfalloc_early() here guarantees that the
             * page tables are allocated contiguously, and that they keep the
             * same address once paging is enabled. */
            pte_t *page_table = (pte_t *)boot_page_alloc_early(boot_alloc);

            if(first_directory && idx == start_index) {
                /* remember the address of the first page table for use by
                 * vm_map() later */
                global_page_tables = page_table;
            }

            set_pte(
                get_pte_with_offset(page_directory, idx),
                EARLY_PTR_TO_PHYS_ADDR(page_table),
                VM_FLAG_PRESENT | VM_FLAG_READ_WRITE);

            /* clear page table */
            for(idy = 0; idy < page_table_entries; ++idy) {
                clear_pte( get_pte_with_offset(page_table, idy) );
            }
        }
    }
}

addr_space_t *vm_create_initial_addr_space(
        bool             use_pae,
        boot_alloc_t    *boot_alloc) {

    if(use_pae) {
        return vm_pae_create_initial_addr_space(boot_alloc);
    }
    else {
        return vm_x86_create_initial_addr_space(boot_alloc);
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
