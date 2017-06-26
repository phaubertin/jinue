#include <assert.h>
#include <hal/cpu.h>
#include <hal/cpu_data.h>
#include <hal/kernel.h>
#include <hal/vga.h>
#include <hal/vm.h>
#include <hal/vm_x86.h>
#include <hal/x86.h>
#include <pfalloc.h>
#include <printk.h>
#include <slab.h>
#include <stdint.h>
#include <vm_alloc.h>


bool vm_use_pae;

size_t page_table_entries;

pte_t *global_page_tables;

addr_space_t initial_addr_space;

slab_cache_t *addr_space_cache;

/** page table entry offset of virtual (linear) address */
unsigned int (*page_table_offset_of)(addr_t);

unsigned int (*page_directory_offset_of)(addr_t);

static vm_alloc_t __global_page_allocator;

/** global page allocator (region 0..KLIMIT) */
vm_alloc_t *global_page_allocator;


void vm_init(void) {
    addr_t           addr;
    addr_space_t    *addr_space;
    uint32_t         temp;
    
    if(cpu_features & CPU_FEATURE_PAE) {
        printk("Processor supports Physical Address Extension (PAE).\n");
        
        /** TODO: change me once PAE support is implemented */
        vm_x86_set_pointers();
        vm_use_pae = false;
    }
    else {
        vm_x86_set_pointers();
        vm_use_pae = false;
    }
    
    /* create initial address space */
    addr_space = vm_create_initial_addr_space();
    
    /* perform 1:1 mapping of text video memory */
    for(addr = (addr_t)VGA_TEXT_VID_BASE; addr < (addr_t)VGA_TEXT_VID_TOP; addr += PAGE_SIZE) {
        vm_map_early((addr_t)addr, (addr_t)addr, VM_FLAG_KERNEL | VM_FLAG_READ_WRITE);
    }
    
    /** below this point, it is no longer safe to call pfalloc_early() */
    use_pfalloc_early = false;
    
    /* perform 1:1 mapping of kernel image and data
    
       note: page tables for memory region (0..KLIMIT) are contiguous
             in physical memory */
    for(addr = (addr_t)&kernel_start; addr < kernel_region_top; addr += PAGE_SIZE) {
        vm_map_early((addr_t)addr, (addr_t)addr, VM_FLAG_KERNEL | VM_FLAG_READ_WRITE);
    }
    
    /* switch to new address space */
    vm_switch_addr_space(addr_space);
    
    /* enable paging */
    temp = get_cr0();
    temp |= X86_FLAG_PG;
    set_cr0(temp);
    
    /* initialize global page allocator (region 0..KLIMIT)
      
       note: We skip the first page (i.e. actually allocate the region PAGE_SIZE..KLIMIT)
             for two reasons:
                - We want null pointer deferences to generate a page fault instead of
                  being more or less silently ignored (read) or overwriting something
                  potentially important (write).
                - We want to ensure nothing interesting (e.g. address space
                  management data structures) can have NULL as its valid
                  address.
                  
      This allocator manages the region from the start of the address space (exluding
      the first page) up to KLIMIT, with two holes: the VGA video buffer and the kernel. */
    global_page_allocator = &__global_page_allocator;
    vm_alloc_init_piecewise(global_page_allocator, NULL, (addr_t)PAGE_SIZE,         (addr_t)VGA_TEXT_VID_BASE, (addr_t)KLIMIT);
    vm_alloc_add_region(global_page_allocator,           (addr_t)VGA_TEXT_VID_TOP,  (addr_t)&kernel_start);
    vm_alloc_add_region(global_page_allocator,           (addr_t)kernel_region_top, (addr_t)KLIMIT);
    
    /* create slab cache for address space descriptors */
    addr_space_cache = slab_cache_create(
        "addr_space_cache",
        sizeof(addr_space_t),
        0,
        NULL,
        NULL,
        0 );
}


/** 
    Map a page frame (physical page) to a virtual memory page.
    @param addr_space address space in which to map, can be NULL for global mappings (vaddr < KLIMIT)
    @param vaddr virtual address of mapping
    @param paddr address of page frame
    @param flags flags used for mapping (see VM_FLAG_x constants in vm.h)
*/
void vm_map(addr_space_t *addr_space, addr_t vaddr, pfaddr_t paddr, int flags) {
    pte_t *pte, *pde;
    pte_t *page_table;
    pte_t *page_directory;
    pfaddr_t pf_page_table;
    unsigned int idx;
    
    /** ASSERTION: we assume vaddr is aligned on a page boundary */
    assert( page_offset_of(vaddr) == 0 );
    
    if(vaddr < KLIMIT) {
        /* fast path for global allocations by the kernel:
         *  - The page tables for the region below KLIMIT are
         *    pre-allocated during the creation of the address space, so
         *    no need to check and allocate them;
         *  - The page tables are mapped contiguously at a known
         *    location during initialization, so no need to find and map
         *    them;
         *  - The mappings for this region are global, so we don't care
         *    about the specified address space.  */
        
        pte = get_pte_with_offset(global_page_tables, page_number_of(vaddr));
        set_pte(pte, paddr, flags | VM_FLAG_PRESENT);
    }
    else {
        /** ASSERTION: addr_space cannot be NULL for non-global mappings */
        assert(addr_space != NULL);
        
        /* map page directory temporarily */
        page_directory = (pte_t *)vm_alloc(global_page_allocator);
        vm_map_global((addr_t)page_directory, addr_space->top_level.pd, VM_FLAGS_PAGE_TABLE);
        
        /* lookup page directory entry */
        pde = get_pte_with_offset(page_directory, page_directory_offset_of(vaddr));
        
        /* map page table, allocate as needed */
        page_table = (pte_t *)vm_alloc(global_page_allocator);
        
        if( get_pte_flags(pde) & VM_FLAG_PRESENT ) {
            vm_map_global((addr_t)page_table, get_pte_pfaddr(pde), VM_FLAGS_PAGE_TABLE);
        }
        else {
            pf_page_table = pfalloc();
            
            vm_map_global((addr_t)page_table, pf_page_table, VM_FLAGS_PAGE_TABLE);
            
            /* zero content of page table */
            for(idx = 0; idx < page_table_entries; ++idx) {
                clear_pte( get_pte_with_offset(page_table, idx) );
            }
            
            /* link page table in page directory */
            set_pte(pde, pf_page_table, VM_FLAG_USER | VM_FLAG_READ_WRITE | VM_FLAG_PRESENT);
        }
        
        pte = get_pte_with_offset(page_table, page_table_offset_of(vaddr));
        set_pte(pte, paddr, flags | VM_FLAG_PRESENT);
        
        vm_unmap_global((addr_t)page_directory);
        vm_unmap_global((addr_t)page_table);
    }
        
    /* invalidate TLB entry for newly mapped page */
    invalidate_tlb(vaddr);
}

/**
    Unmap a page from virtual memory.
    @param addr_space address space from which to unmap, can be NULL for global mappings (addr < KLIMIT)
    @param addr address of page to unmap
*/
void vm_unmap(addr_space_t *addr_space, addr_t addr) {
    pte_t *pte, *pde;
    pte_t *page_table;
    pte_t *page_directory;
    
    /** ASSERTION: we assume addr is aligned on a page boundary */
    assert( page_offset_of(addr) == 0 );
    
    if(addr < KLIMIT) {
        /* fast path for global allocations by the kernel (see vm_map())
         *
         * Performance optimization: vm_unmap is a no-op for kernel mappings
         * when compiling non-debug.
         * 
         * When compiling in debug mode, we still do the Right Thing(TM) so
         * that buggy code which accesses pages it shouldn't trigger a page
         * fault, to help track such accesses. */
#ifdef NDEBUG
        return;
#endif
        
        pte = get_pte_with_offset(global_page_tables, page_number_of(addr));
        clear_pte(pte);
    }
    else {
        /** ASSERTION: addr_space cannot be NULL for non-global mappings */
        assert(addr_space != NULL);
        
        /* map page directory temporarily */
        page_directory = (pte_t *)vm_alloc(global_page_allocator);
        vm_map_global((addr_t)page_directory, addr_space->top_level.pd, VM_FLAGS_PAGE_TABLE);
        
        /* lookup page directory entry */
        pde = get_pte_with_offset(page_directory, page_directory_offset_of(addr));
        
        if( ! (get_pte_flags(pde) & VM_FLAG_PRESENT) ) {
            /* nothing to do */
            vm_unmap_global((addr_t)page_directory);
            return;
        }
        
        /* map page table */
        page_table = (pte_t *)vm_alloc(global_page_allocator);
        vm_map_global((addr_t)page_table, get_pte_pfaddr(pde), VM_FLAGS_PAGE_TABLE);
                
        /* clear page table entry */
        pte = get_pte_with_offset(page_table, page_table_offset_of(addr));
        clear_pte(pte);
        
        vm_unmap_global((addr_t)page_directory);
        vm_unmap_global((addr_t)page_table);
    }
    
    /* invalidate TLB entry for newly mapped page */
    invalidate_tlb(addr);
}

pfaddr_t vm_lookup_pfaddr(addr_space_t *addr_space, addr_t addr) {
    pte_t *pte, *pde;
    pte_t *page_table;
    pte_t *page_directory;
    pfaddr_t pfaddr;
    
    if(addr < KLIMIT) {
        /* fast path for global allocations by the kernel (see vm_map()) */
        
        pte = get_pte_with_offset(global_page_tables, page_number_of(addr));
        
        /** ASSERTION: there is a page table entry marked present for this address */
        assert( get_pte_flags(pte) & VM_FLAG_PRESENT );
        
        pfaddr = get_pte_pfaddr(pte);
    }
    else {
        /** ASSERTION: addr_space cannot be NULL for non-global mappings */
        assert(addr_space != NULL);
        
        /* map page directory temporarily */
        page_directory = (pte_t *)vm_alloc(global_page_allocator);
        vm_map_global((addr_t)page_directory, addr_space->top_level.pd, VM_FLAGS_PAGE_TABLE);
        
        /* lookup page directory entry */
        pde = get_pte_with_offset(page_directory, page_directory_offset_of(addr));
        
        /** ASSERTION: there is a page directory entry marked present for this address */
        assert( get_pte_flags(pde) & VM_FLAG_PRESENT );
        
        /* map page table */
        page_table = (pte_t *)vm_alloc(global_page_allocator);
        vm_map_global((addr_t)page_table, get_pte_pfaddr(pde), VM_FLAGS_PAGE_TABLE);
        
        pte = get_pte_with_offset(page_table, page_table_offset_of(addr));
                
        /** ASSERTION: there is a page table entry marked present for this address */
        assert( get_pte_flags(pte) & VM_FLAG_PRESENT );
        
        pfaddr = get_pte_pfaddr(pte);
        
        vm_unmap_global((addr_t)page_directory);
        vm_unmap_global((addr_t)page_table);
    }
    
    return pfaddr;
}

void vm_change_flags(addr_space_t *addr_space, addr_t addr, int flags) {
    pte_t *pte, *pde;
    pte_t *page_table;
    pte_t *page_directory;
    
    /** ASSERTION: we assume addr is aligned on a page boundary */
    assert( page_offset_of(addr) == 0 );
    
    if(addr < KLIMIT) {
        /* fast path for global allocations by the kernel (see vm_map()) */
        
        pte = get_pte_with_offset(global_page_tables, page_number_of(addr));
        
        /** ASSERTION: there is a page table entry marked present for this address */
        assert( get_pte_flags(pte) & VM_FLAG_PRESENT );
        
        /* perform the flags change */
        set_pte_flags(pte, flags | VM_FLAG_PRESENT);
    }
    else {
        /** ASSERTION: addr_space cannot be NULL for non-global mappings */
        assert(addr_space != NULL);
        
        /* map page directory temporarily */
        page_directory = (pte_t *)vm_alloc(global_page_allocator);
        vm_map_global((addr_t)page_directory, addr_space->top_level.pd, VM_FLAGS_PAGE_TABLE);
        
        /* lookup page directory entry */
        pde = get_pte_with_offset(page_directory, page_directory_offset_of(addr));
        
        /** ASSERTION: there is a page directory entry marked present for this address */
        assert( get_pte_flags(pde) & VM_FLAG_PRESENT );
        
        /* map page table */
        page_table = (pte_t *)vm_alloc(global_page_allocator);
        vm_map_global((addr_t)page_table, get_pte_pfaddr(pde), VM_FLAGS_PAGE_TABLE);
        
        pte = get_pte_with_offset(page_table, page_table_offset_of(addr));
                
        /** ASSERTION: there is a page table entry marked present for this address */
        assert( get_pte_flags(pte) & VM_FLAG_PRESENT );
        
        /* perform the flags change */
        set_pte_flags(pte, flags | VM_FLAG_PRESENT);
        
        vm_unmap_global((addr_t)page_directory);
        vm_unmap_global((addr_t)page_table);
    }
    
    /* invalidate TLB entry for the affected page */
    invalidate_tlb(addr);
}

void vm_map_early(addr_t vaddr, addr_t paddr, int flags) {
    pte_t *pte;
    
    /** ASSERTION: we are mapping in the 0..KLIMIT region */
    assert(vaddr < KLIMIT);
    
    /** ASSERTION: we assume vaddr is aligned on a page boundary */
    assert( page_offset_of(vaddr) == 0 );
    
    /** ASSERTION: we assume paddr is aligned on a page boundary */
    assert( page_offset_of(paddr) == 0 );
    
    pte = get_pte_with_offset(global_page_tables, page_number_of(vaddr));
    set_pte(pte, PTR_TO_PFADDR(paddr), flags | VM_FLAG_PRESENT);
}

addr_space_t *vm_create_addr_space(void) {
    unsigned int idx;
    pfaddr_t pfaddr;
    pte_t *page_directory;
    pte_t *template;
    addr_space_t *addr_space;
    
    /* allocate and map new page directory */
    page_directory = (pte_t *)vm_alloc(global_page_allocator);
    pfaddr = pfalloc();
    vm_map_global((addr_t)page_directory, pfaddr, VM_FLAGS_PAGE_TABLE);
    
    /* use initial address space page directory as template for the
     * global allocations region (0..KLIMIT) */
    template = (pte_t *)vm_alloc(global_page_allocator);
    vm_map_global((addr_t)template, initial_addr_space.top_level.pd, VM_FLAGS_PAGE_TABLE);
    
    /* the page tables for the global allocations region (0..KLIMIT) are
     * the same in all address spaces, so copy them from the template */
    for(idx = 0; idx < page_directory_offset_of(KLIMIT); idx++) {
        copy_pte(
            get_pte_with_offset(page_directory, idx),
            get_pte_with_offset(template, idx)
        );
    }
    
    /* clear remaining entries: these page tables are allocated on demand */
    while(idx < page_table_entries) {
        clear_pte( get_pte_with_offset(page_directory, idx) );
        ++idx;
    }
    
    addr_space = slab_cache_alloc(addr_space_cache);
    
    vm_unmap_global((addr_t)page_directory);
    vm_unmap_global((addr_t)template);
    
    addr_space->top_level.pd = pfaddr;
    addr_space->cr3          = (uint32_t)PFADDR_TO_PTR(pfaddr);
    
    return addr_space;
}

addr_space_t *vm_create_initial_addr_space(void) {
    unsigned int idx, idy;
    pte_t *page_directory;
    pte_t *page_table;
    
    /* Allocate the first page directory. Since paging is not yet
       enabled, virtual and physical addresses are the same.  */
    page_directory = (pte_t *)pfalloc_early();
        
    /* allocate page tables for kernel data/code region (0..KLIMIT) */
    for(idx = 0; idx < page_directory_offset_of(KLIMIT); ++idx) {
        /* allocate the page table
         * 
         * Note that the use of pfalloc_early() here guarantees that the
         * page table are allocated contiguously, and that they keep the
         * same address once paging is enabled. */
        page_table = (pte_t *)pfalloc_early();
        
        if(idx == 0) {
            /* remember the address of the first page table for use by
             * vm_map() later */
            global_page_tables = page_table;
        }
            
        set_pte(
            get_pte_with_offset(page_directory, idx),
            PTR_TO_PFADDR(page_table),
            VM_FLAG_PRESENT | VM_FLAG_KERNEL | VM_FLAG_READ_WRITE );
        
        /* clear page table */
        for(idy = 0; idy < page_table_entries; ++idy) {
            clear_pte( get_pte_with_offset(page_table, idy) );
        }
    }
    
    /* clear remaining entries: these page tables are allocated on demand */
    while(idx < page_table_entries) {
        clear_pte( get_pte_with_offset(page_directory, idx) );
        ++idx;
    }
    
    initial_addr_space.top_level.pd = PTR_TO_PFADDR(page_directory);
    initial_addr_space.cr3          = (uint32_t)page_directory;
    
    return &initial_addr_space;
}

void vm_destroy_addr_space(addr_space_t *addr_space) {
    pte_t *pte;
    pte_t *page_directory;
    unsigned int idx;
    
    /** ASSERTION: address space must not be NULL */
    assert(addr_space != NULL);

    /** ASSERTION: the initial address space should not be destroyed */
    assert(addr_space != &initial_addr_space);
    
    /** ASSERTION: the current address space should not be destroyed */
    assert( addr_space != get_current_addr_space() );
    
    page_directory = (pte_t *)vm_alloc(global_page_allocator);
    vm_map_global((addr_t)page_directory, addr_space->top_level.pd, VM_FLAGS_PAGE_TABLE);
    
    for(idx = page_directory_offset_of(KLIMIT); idx < page_table_entries; ++idx) {
        pte = get_pte_with_offset(page_directory, idx);
        
        if( get_pte_flags(pte) & VM_FLAG_PRESENT ) {
            pffree( get_pte_pfaddr(pte) );
        }
    }
    
    vm_unmap_global((addr_t)page_directory);
    pffree(addr_space->top_level.pd);
    slab_cache_free(addr_space);
}

void vm_switch_addr_space(addr_space_t *addr_space) {
    set_cr3(addr_space->cr3);
}
