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
#include <hal/kernel.h>
#include <hal/pfaddr.h>
#include <hal/vga.h>
#include <hal/vm.h>
#include <hal/vm_private.h>
#include <hal/x86.h>
#include <assert.h>
#include <pfalloc.h>
#include <printk.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <vm_alloc.h>


pte_t *global_page_tables;

addr_space_t initial_addr_space;

static vm_alloc_t __global_page_allocator;

/** global page allocator (region 0..KLIMIT) */
vm_alloc_t *global_page_allocator;

/* Note on naming conventions:
 * 
 * Each function that starts with a vm_x86_ prefix (e.g. vm_x86_some_func()) is
 * a function that is specific to the 32-bit paging (i.e. non-PAE) code. Each such
 * function has a alternative PAE implementation in vm_pae.c that starts with the
 * vm_pae_ prefix (e.g. vm_pae_some_func()) and is accessed through a function
 * pointer that allows the implementation to be selected during boot intialization.
 * Function pointers for this purpose do not have a vm_... prefix (e.g. some_func).
 * 
 * vm_x86_... functions must be called directly only by other vm_x86_... functions.
 * Otherwise, they must be called through the function pointer. (Obviously, the
 * same holds for vm_pae_xxx functions.)
 * 
 * Functions whose name start with the vm_ prefix, but not vm_x86_ or vm_pae_,
 * are "normal" functions that can be called directly.
 * 
 * Note on header files:
 * 
 * The vm.h header file provides the public interface for this file.
 * 
 * The vm_private.h header file provides private definitions that are shared by
 * this file and hal/vm_pae.c
 * 
 * The vm_pae.h header file provides declarations for the functions defined in
 * hal/vm_pae.c.
 * */

void vm_boot_init(const boot_info_t *boot_info, bool use_pae, cpu_data_t *cpu_data) {
    addr_t           addr;
    addr_space_t    *addr_space;
    
    if(use_pae) {
        printk("Enabling Physical Address Extension (PAE).\n");
        vm_pae_boot_init();
    }
    
    /* create initial address space */
    addr_space = vm_create_initial_addr_space(use_pae);
    
    /** below this point, it is no longer safe to call pfalloc_early() */
    use_pfalloc_early = false;
    
    /* perform 1:1 mapping of kernel image and data */
    for(addr = (addr_t)boot_info->image_start; addr < kernel_region_top; addr += PAGE_SIZE) {
        vm_map_early((addr_t)addr, EARLY_PTR_TO_PFADDR(addr), VM_FLAG_KERNEL | VM_FLAG_READ_WRITE);
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
    addr_t kernel_vm_top = kernel_region_top;
    addr = (addr_t)VGA_TEXT_VID_BASE;
    
    addr_t vga_text_base = kernel_vm_top;

    while(addr < (addr_t)VGA_TEXT_VID_TOP) {
        vm_map_early(kernel_vm_top, ADDR_TO_PFADDR((uintptr_t)addr), VM_FLAG_KERNEL | VM_FLAG_READ_WRITE);
        kernel_vm_top   += PAGE_SIZE;
        addr            += PAGE_SIZE;
    }

    /* remap VGA text buffer
     * 
     * Note: after the call to vga_set_base_addr() below until we switch to the
     * new address space, VGA output is not possible. Calling printk() will cause
     * a kernel panic due to a page fault (and the panic handler calls printk()). */
    printk("Remapping text video memory at 0x%x\n", kernel_vm_top);
    
    vga_set_base_addr(vga_text_base);
    
    if(use_pae) {
        /* If we are enabling PAE, this is where the switch to the new page
         * tables actually happens instead of at the call to vm_switch_addr_space()
         * as would be expected.
         * 
         * From Intel 64 and IA-32 Architectures Software Developer's Manual
         * Volume 3: System Programming Guide, section 4.4.1 "PDPTE Registers":
         * 
         *  " The logical processor loads [the PDPTE] registers from the PDPTEs
         *    in memory as part of certain operations:
         *      * If PAE paging would be in use following an execution of MOV to
         *        CR0 or MOV to CR4 (see Section 4.1.1) and the instruction is
         *        modifying any of (...) CR4.PAE, (...); then the PDPTEs are
         *        loaded from the address in CR3. "
         * 
         * There are bootstrapping issues when enabling PAE while paging is enabled.
         * See the comment at the top of the vm_pae_create_initial_addr_space()
         * function in vm_pae.c for more detail. */
        vm_pae_enable();
    }

    /* switch to new address space */
    vm_switch_addr_space(addr_space, cpu_data);
}

void vm_boot_postinit(const boot_info_t *boot_info, bool use_pae) {
    /* initialize global page allocator (region starting at KLIMIT)
     * 
     * TODO Some work needs to be done in the page allocator to support allocating
     * up to the top of memory (i.e. 0x100000000, which cannot be represented on
     * 32 bits). In the mean time, we leave a 4MB gap. */
    global_page_allocator = &__global_page_allocator;
    vm_alloc_init_allocator(global_page_allocator, (addr_t)KLIMIT, (addr_t)0 - 4 * MB);

    vm_alloc_add_region(global_page_allocator, (addr_t)KLIMIT,          (addr_t)boot_info->image_start);
    vm_alloc_add_region(global_page_allocator, (addr_t)KLIMIT + 4 * MB, (addr_t)0 - 4 * MB);
    
    /* create slab cache to allocate PDPTs
     * 
     * This must be done after the global page allocator has been initialized
     * because the slab allocator needs to allocate a slab to allocate the new
     * slab cache on the slab cache cache.
     * 
     * This must be done before the first time vm_create_addr_space() is called. */
    if(use_pae) {
        vm_pae_create_pdpt_cache();
    }
}

/** 
    Lookup and map the page directory for a specified address and address space.
    
    This is the implementation for standard 32-bit (i.e. non-PAE) paging. This
    means that there is only one preallocated page directory, so the addr and
    create_as_needed arguments are both irrelevant.
    
    Important note: it is the caller's responsibility to unmap and free the returned
    page directory when it is done with it.
    
    @param addr_space address space in which the address is looked up.
    @param addr address to look up
    @param create_as_need Whether a page table is allocated if it does not exist
*/
static pte_t *vm_x86_lookup_page_directory(addr_space_t *addr_space, void *addr, bool create_as_needed) {
    pte_t *page_directory = (pte_t *)vm_alloc(global_page_allocator);
    vm_map_kernel((addr_t)page_directory, addr_space->top_level.pd, VM_FLAG_READ_WRITE);
        
    return page_directory;
}

/** 
    Lookup a page table entry for a specified address and address space.
    
    If the create_as_needed argument is true, new page tables are allocated as
    needed. Otherwise, NULL is returned if there is currently no page table for
    the specified address and address space.
    
    If a non-NULL value is returned, it is the caller's responsibility to call
    vm_free_page_table_entry() when they are done with it.
    
    Important note: This function temporarily maps the page table that contains
    the returned page table entry and allocates a page of virtual memory for that
    purpose. The caller must call the vm_free_page_table_entry() function to free
    that mapping when it is done with the page table entry.
    
    @param addr_space address space in which the address is looked up.
    @param addr address to look up
    @param create_as_need Whether a page table is allocated if it does not exist
*/
static pte_t *vm_lookup_page_table_entry(addr_space_t *addr_space, void *addr, bool create_as_needed) {
    /** ASSERTION: we assume addr is aligned on a page boundary */
    assert( page_offset_of(addr) == 0 );
    
    if(is_fast_map_pointer(addr)) {
        /* Fast path for global allocations by the kernel:
         *  - The page tables for the region above KLIMIT are pre-allocated
         *    during the creation of the address space, so there no need to
         *    check or to allocate them;
         *  - The page tables are mapped contiguously at a known location
         *    during initialization, so no need to find and map them, and they
         *    can be indexed as a single big page table;
         *  - The mappings for this region are global, so we don't care
         *    about the specified address space.  */
        return get_pte_with_offset(global_page_tables, page_number_of((uintptr_t)addr - KLIMIT));
    }
    else {
        pte_t *page_table;
        pte_t *pte;
    
        /** ASSERTION: addr_space cannot be NULL for non-global mappings */
        assert(addr_space != NULL);
        
        /* map page directory temporarily */
        pte_t *page_directory = lookup_page_directory(addr_space, addr, create_as_needed);
        
        if(page_directory == NULL) {
            /* no page directory */
            return NULL;
        }
      
        /* lookup page directory entry */
        pte_t *pde = get_pte_with_offset(page_directory, page_directory_offset_of(addr));

        if( get_pte_flags(pde) & VM_FLAG_PRESENT ) {
            /* map page table */
            page_table = (pte_t *)vm_alloc(global_page_allocator);
            
            vm_map_kernel((addr_t)page_table, get_pte_pfaddr(pde), VM_FLAG_READ_WRITE);
            
            /* get page table entry */
            pte = get_pte_with_offset(page_table, page_table_offset_of(addr));
        }
        else {
            if(create_as_needed) {
                pfaddr_t pf_page_table;
                
                /* allocate a new page table and map it */
                page_table      = (pte_t *)vm_alloc(global_page_allocator);
                pf_page_table   = pfalloc();
            
                vm_map_kernel((addr_t)page_table, pf_page_table, VM_FLAG_READ_WRITE);
                
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
                
                set_pte(pde, pf_page_table, access_flags | VM_FLAG_READ_WRITE | VM_FLAG_PRESENT);
                
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
        vm_free(global_page_allocator, (addr_t)page_directory);
        
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
        vm_free(global_page_allocator, page_table);
    }
}

/** 
    Map a page frame (physical page) to a virtual memory page.
    @param addr_space address space in which to map, can be NULL for global mappings (vaddr >= KLIMIT)
    @param vaddr virtual address of mapping
    @param paddr address of page frame
    @param flags flags used for mapping (see VM_FLAG_x constants in vm.h)
*/
static void vm_map(addr_space_t *addr_space, addr_t vaddr, pfaddr_t paddr, int flags) {
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
    
#ifdef NDEBUG
    /* Performance optimization: vm_unmap is a no-op for kernel mappings when
     * compiling non-debug.
     * 
     * When compiling in debug mode, the unmap operation is actually performed
     * to help detect use-after-unmap bugs. */
    if(is_kernel_pointer(addr)) {
        return;
    }
#endif

    pte_t *pte = vm_lookup_page_table_entry(addr_space, addr, false);
    
    if(pte != NULL) {
        clear_pte(pte);
    
        vm_free_page_table_entry(addr, pte);
        
        /* invalidate TLB entry for newly mapped page */
        invalidate_tlb(addr);
    }
}

void vm_map_kernel(addr_t vaddr, pfaddr_t paddr, int flags) {
    vm_map(NULL, vaddr, paddr, flags | VM_FLAG_KERNEL);
}

void vm_map_user(addr_space_t *addr_space, addr_t vaddr, pfaddr_t paddr, int flags) {
    vm_map(addr_space, vaddr, paddr, flags | VM_FLAG_USER);
}

void vm_unmap_kernel(addr_t addr) {
    vm_unmap(NULL, addr);
}

void vm_unmap_user(addr_space_t *addr_space, addr_t addr) {
    vm_unmap(addr_space, addr);
}

pfaddr_t vm_lookup_pfaddr(addr_space_t *addr_space, addr_t addr) {
    pte_t *pte = vm_lookup_page_table_entry(addr_space, addr, false);
    
    /** ASSERTION: there is a page table entry marked present for this address */
    assert(pte != NULL && (get_pte_flags(pte) & VM_FLAG_PRESENT));
    
    pfaddr_t pfaddr = get_pte_pfaddr(pte);
    
    vm_free_page_table_entry(addr, pte);
    
    return pfaddr;
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

void vm_map_early(addr_t vaddr, pfaddr_t paddr, int flags) {
    pte_t *pte;

    /** ASSERTION: we are mapping in the kernel region */
    assert( is_fast_map_pointer(vaddr) );
    
    /** ASSERTION: we assume vaddr is aligned on a page boundary */
    assert( page_offset_of(vaddr) == 0 );
    
    pte = get_pte_with_offset(global_page_tables, page_number_of( EARLY_VIRT_TO_PHYS((uintptr_t)vaddr) ));
    set_pte(pte, paddr, flags | VM_FLAG_PRESENT);
}

pfaddr_t vm_clone_page_directory(pfaddr_t template_pfaddr, unsigned int start_index) {
    unsigned int idx;
    pfaddr_t pfaddr;
    pte_t *page_directory;
    pte_t *template;
    
    /* allocate and map new page directory */
    page_directory = (pte_t *)vm_alloc(global_page_allocator);
    pfaddr = pfalloc();
    vm_map_kernel((addr_t)page_directory, pfaddr, VM_FLAG_READ_WRITE);
    
    /* map page directory template */
    template = (pte_t *)vm_alloc(global_page_allocator);
    vm_map_kernel((addr_t)template, template_pfaddr, VM_FLAG_READ_WRITE);

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
    
    return pfaddr;
}

static addr_space_t *vm_x86_create_addr_space(addr_space_t *addr_space) {
    /* Create a new page directory where entries for the address range starting
     * at KLIMIT are copied from the initial address space. The mappings starting
     * at KLIMIT belong to the kernel and are identical in all address spaces. */
    pfaddr_t pfaddr = vm_clone_page_directory(
            initial_addr_space.top_level.pd,
            page_directory_offset_of((addr_t)KLIMIT));
    
    addr_space->top_level.pd = pfaddr;
    addr_space->cr3          = (uint32_t)PFADDR_TO_ADDR(pfaddr);
    
    return addr_space;
}

addr_space_t *vm_create_addr_space(addr_space_t *addr_space) {
    return create_addr_space(addr_space);
}

pte_t *vm_allocate_page_directory(unsigned int start_index, bool first_pd) {
    unsigned int idx, idy;
    pte_t *page_directory;
    pte_t *page_table;
    
    /* Allocate page directory. */
    page_directory = (pte_t *)pfalloc_early();
    
    /* clear user space page directory entries */
    for(idx = 0; idx < start_index; ++idx) {
        clear_pte( get_pte_with_offset(page_directory, idx) );
    }
    
    /* allocate page tables for kernel data/code region (above KLIMIT) */
    for(idx = start_index; idx < page_table_entries; ++idx) {
        /* allocate the page table
         * 
         * Note that the use of pfalloc_early() here guarantees that the
         * page table are allocated contiguously, and that they keep the
         * same address once paging is enabled. */
        page_table = (pte_t *)pfalloc_early();
        
        if(first_pd && idx == start_index) {
            /* remember the address of the first page table for use by
             * vm_map() later */
            global_page_tables = page_table;
        }
            
        set_pte(
            get_pte_with_offset(page_directory, idx),
            EARLY_PTR_TO_PFADDR(page_table),
            VM_FLAG_PRESENT | VM_FLAG_READ_WRITE );
        
        /* clear page table */
        for(idy = 0; idy < page_table_entries; ++idy) {
            clear_pte( get_pte_with_offset(page_table, idy) );
        }
    }
    
    return page_directory;
}

addr_space_t *vm_x86_create_initial_addr_space(void) {
    unsigned int klimit_pd_index = page_directory_offset_of((addr_t)KLIMIT);
    
    pte_t *page_directory = vm_allocate_page_directory(klimit_pd_index, true);
    
    initial_addr_space.top_level.pd = EARLY_PTR_TO_PFADDR(page_directory);
    initial_addr_space.cr3          = EARLY_VIRT_TO_PHYS((uintptr_t)page_directory);
    
    return &initial_addr_space;
}

addr_space_t *vm_create_initial_addr_space(bool use_pae) {
    if(use_pae) {
        return vm_pae_create_initial_addr_space();
    }
    else {
        return vm_x86_create_initial_addr_space();
    }
}

void vm_destroy_page_directory(pfaddr_t pdpfaddr, unsigned int from_index, unsigned int to_index) {
    unsigned int idx;
   
    pte_t *page_directory = (pte_t *)vm_alloc(global_page_allocator);
    vm_map_kernel((addr_t)page_directory, pdpfaddr, VM_FLAG_READ_WRITE);
    
    /* be careful not to free the kernel page tables */
    for(idx = from_index; idx < to_index; ++idx) {
        pte_t *pte = get_pte_with_offset(page_directory, idx);
        
        if(get_pte_flags(pte) & VM_FLAG_PRESENT) {
            pffree( get_pte_pfaddr(pte) );
        }
    }
    
    vm_unmap_kernel((addr_t)page_directory);
    pffree(pdpfaddr);
}

static void vm_x86_destroy_addr_space(addr_space_t *addr_space) {
    vm_destroy_page_directory(
            addr_space->top_level.pd,
            /* Free page tables for addresses 0..KLIMIT, be careful not to free
             * the kernel page tables starting at KLIMIT. */
            0,
            page_directory_offset_of((addr_t)KLIMIT));
}

void vm_destroy_addr_space(addr_space_t *addr_space) {
    /** ASSERTION: address space must not be NULL */
    assert(addr_space != NULL);

    /** ASSERTION: the initial address space should not be destroyed */
    assert(addr_space != &initial_addr_space);
    
    /** ASSERTION: the current address space should not be destroyed */
    assert( addr_space != get_current_addr_space() );
    
    destroy_addr_space(addr_space);
}

void vm_switch_addr_space(addr_space_t *addr_space, cpu_data_t *cpu_data) {
    set_cr3(addr_space->cr3);

    cpu_data->current_addr_space = addr_space;
}

/* Above this point, functions can pass around pointers to page table entries
 * (pte_t) but must never dereference them. This is because, depending on
 * whether Physical Address Extension (PAE) is enabled or not, these pointers
 * can refer to objects that match either the struct definition just below
 * (32-bit entries) or the PAE definition (64-bit entries).
 * 
 * This structure is declared late in the file on purpose to help detect
 * inappropriate pointer dereferences. */
struct pte_t {
    uint32_t entry;
};


static unsigned int vm_x86_page_table_offset_of(addr_t addr) {
    return PAGE_TABLE_OFFSET_OF(addr);
}

static unsigned int vm_x86_page_directory_offset_of(addr_t addr) {
    return PAGE_DIRECTORY_OFFSET_OF(addr);
}

static pte_t *vm_x86_get_pte_with_offset(pte_t *pte, unsigned int offset) {
    return &pte[offset];
}

static void vm_x86_set_pte(pte_t *pte, pfaddr_t paddr, int flags) {
    /** TODO: check paddr for 4GB limit */
    pte->entry = ((uint32_t)paddr << PFADDR_SHIFT) | flags;
}

static void vm_x86_set_pte_flags(pte_t *pte, int flags) {
    pte->entry = (pte->entry & ~PAGE_MASK) | flags;
}

static int vm_x86_get_pte_flags(pte_t *pte) {
    return pte->entry & PAGE_MASK;
}

static pfaddr_t vm_x86_get_pte_pfaddr(pte_t *pte) {
    return (pte->entry & ~PAGE_MASK) >> PFADDR_SHIFT;
}

static void vm_x86_clear_pte(pte_t *pte) {
    pte->entry = 0;
}

static void vm_x86_copy_pte(pte_t *dest, pte_t *src) {
    dest->entry = src->entry;
}

size_t page_table_entries                                       = (size_t)PAGE_TABLE_ENTRIES;

addr_space_t *(*create_addr_space)(addr_space_t *)              = vm_x86_create_addr_space;

void (*destroy_addr_space)(addr_space_t *)                      = vm_x86_destroy_addr_space;

/** page table entry offset of virtual (linear) address */
unsigned int (*page_table_offset_of)(addr_t)                    = vm_x86_page_table_offset_of;

unsigned int (*page_directory_offset_of)(addr_t)                = vm_x86_page_directory_offset_of;

pte_t *(*lookup_page_directory)(addr_space_t *, void *, bool)   = vm_x86_lookup_page_directory;

pte_t *(*get_pte_with_offset)(pte_t *, unsigned int)            = vm_x86_get_pte_with_offset;

void (*set_pte)(pte_t *, pfaddr_t, int)                         = vm_x86_set_pte;

void (*set_pte_flags)(pte_t *, int)                             = vm_x86_set_pte_flags;

int (*get_pte_flags)(pte_t *)                                   = vm_x86_get_pte_flags;

pfaddr_t (*get_pte_pfaddr)(pte_t *)                             = vm_x86_get_pte_pfaddr;

void (*clear_pte)(pte_t *)                                      = vm_x86_clear_pte;

void (*copy_pte)(pte_t *, pte_t *)                              = vm_x86_copy_pte;
