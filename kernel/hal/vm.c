#include <kernel.h>
#include <assert.h>
#include <pfalloc.h>
#include <printk.h>
#include <stdint.h>
#include <vga.h>
#include <vm.h>
#include <vm_alloc.h>
#include <vm_x86.h>
#include <x86.h>


bool vm_use_pae;

addr_t page_directory_addr;

size_t page_table_entries;

/** page table entry offset of virtual (linear) address */
unsigned int (*page_table_offset_of)(addr_t);

unsigned int (*global_page_table_offset_of)(addr_t);

unsigned int (*page_directory_offset_of)(addr_t);

pte_t *(*page_table_of)(addr_t);

pte_t *(*get_pte)(addr_t);

pte_t *(*get_pde)(addr_t);

void (*alloc_page_table)(addr_t);


static vm_alloc_t __global_page_allocator;

/** global page allocator (region 0..KLIMIT) */
vm_alloc_t *global_page_allocator;


void vm_init(void) {
    addr_t addr;
    pte_t *page_directory;
    unsigned long temp;
    
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
    page_directory = vm_create_initial_addr_space();
    
    /* perform 1:1 mapping of text video memory */
    for(addr = (addr_t)VGA_TEXT_VID_BASE; addr < (addr_t)VGA_TEXT_VID_TOP; addr += PAGE_SIZE) {
		vm_map_early((addr_t)addr, (addr_t)addr, VM_FLAG_KERNEL | VM_FLAG_READ_WRITE, page_directory);
	}
	
	/** below this point, it is no longer safe to call pfalloc_early() */
	use_pfalloc_early = false;
	
	/* perform 1:1 mapping of kernel image and data
	
	   note: page tables for memory region (0..KLIMIT) are contiguous
	         in physical memory */
	for(addr = kernel_start; addr < kernel_region_top; addr += PAGE_SIZE) {
		vm_map_early((addr_t)addr, (addr_t)addr, VM_FLAG_KERNEL | VM_FLAG_READ_WRITE, page_directory);
	}
    
    /* enable paging */
	set_cr3( (uint32_t)page_directory );
    
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
	vm_alloc_init_piecewise(global_page_allocator, (addr_t)PAGE_SIZE,         (addr_t)VGA_TEXT_VID_BASE, (addr_t)KLIMIT);
	vm_alloc_add_region(global_page_allocator,     (addr_t)VGA_TEXT_VID_TOP,  (addr_t)kernel_start);
    vm_alloc_add_region(global_page_allocator,     (addr_t)kernel_region_top, (addr_t)KLIMIT);
}


/** 
	Map a page frame (physical page) to a virtual memory page.
	@param vaddr virtual address of mapping
	@param paddr address of page frame
	@param flags flags used for mapping (see VM_FLAG_x constants in vm.h)
*/
void vm_map(addr_t vaddr, pfaddr_t paddr, int flags) {
	pte_t *pte;
	
	/** ASSERTION: we assume vaddr is aligned on a page boundary */
	assert( page_offset_of(vaddr) == 0 );
	
	/** ASSERTION: we assume the page frame is below the 4GB limit */
	assert( PFADDR_CHECK_4GB(paddr) );
    
    /* make sure page table exists: user space page tables are allocated
     * on demand */
    if(vaddr >= PLIMIT) {
        alloc_page_table(vaddr);
    }
	
	/* perform the actual mapping */
	pte = get_pte(vaddr);
    set_pte(pte, paddr, flags | VM_FLAG_PRESENT);
	
	/* invalidate TLB entry for newly mapped page */
	invalidate_tlb(vaddr);
}

/**
	Unmap a page from virtual memory.
	@param addr address of page to unmap
*/
void vm_unmap(addr_t addr) {
	pte_t *pte;
	
	/** ASSERTION: we assume addr is aligned on a page boundary */
	assert( page_offset_of(addr) == 0 );
	
	pte = get_pte(addr);
    clear_pte(pte);
	
	invalidate_tlb(addr);
}

pfaddr_t vm_lookup_pfaddr(addr_t addr) {
	pte_t *pte;
	
	/* get page frame base address from page tables */
	pte = get_pte(addr);
    
    return get_pte_pfaddr(pte);
}

void vm_change_flags(addr_t vaddr, int flags) {
	pte_t *pte;
	
	/** ASSERTION: we assume vaddr is aligned on a page boundary */
	assert( page_offset_of(vaddr) == 0 );
	
	/* get page table entry */
	pte = get_pte(vaddr);
	
	/** ASSERTION: there is a page table entry marked present for this address */
    assert( get_pte_flags(pte) & VM_FLAG_PRESENT );
	
	/* perform the flags change */
    set_pte_flags(pte, flags | VM_FLAG_PRESENT);
	
	/* invalidate TLB entry for the affected page */
	invalidate_tlb(vaddr);	
}

void vm_map_early(addr_t vaddr, addr_t paddr, int flags, pte_t *page_directory) {
	pte_t *page_table;
    pte_t *pte;
	
	/** ASSERTION: we are mapping in the 0..PLIMIT region */
    assert(vaddr < PLIMIT);	
    
    /** ASSERTION: we assume vaddr is aligned on a page boundary */
	assert( page_offset_of(vaddr) == 0 );
	
	/** ASSERTION: we assume paddr is aligned on a page boundary */
	assert( page_offset_of(paddr) == 0 );
	
	/* get page directory entry for vaddr */
    pte        = get_pte_with_offset(page_directory, page_directory_offset_of(vaddr));
    
    /* For early allocations (i.e. before paging is enabled) like the inital page
     * tables, the virtual (linear) address and the physical address are the same. */
    page_table = (pte_t *)(get_pte_pfaddr(pte) << PFADDR_SHIFT);
    
    /* get page table entry for vaddr */
    pte = get_pte_with_offset(page_table, page_table_offset_of(vaddr));
    
    /* perform mapping */
    set_pte(pte, PTR_TO_PFADDR(paddr), flags | VM_FLAG_PRESENT);
}

pte_t *vm_create_initial_addr_space(void) {
    unsigned int idx, idy;
    pte_t *page_directory;
    pte_t *page_table;
    pte_t *page_table_p0;
    
    /* Allocate the first page directory. Since paging is not yet
	   enabled, virtual and physical addresses are the same.  */
	page_directory = (pte_t *)pfalloc_early();
    
    /* prevent compiler from complaining that page_table_p0 might be
     * used uninitialized */
    page_table_p0 = NULL;
    
    /** ASSERTION: page_table_p0 gets assigned in the loop below */
    assert(page_directory_offset_of(PLIMIT) > page_directory_offset_of(KLIMIT));
    
	/* allocate page tables for kernel data/code region (0..PLIMIT) */
	for(idx = 0; idx < page_directory_offset_of(PLIMIT); ++idx) {
        page_table = (pte_t *)pfalloc_early();
            
        set_pte(
            get_pte_with_offset(page_directory, idx),
            PTR_TO_PFADDR(page_table),
            VM_FLAG_PRESENT | VM_FLAG_KERNEL | VM_FLAG_READ_WRITE );
        
        if ( idx == page_directory_offset_of(KLIMIT) ) {
            /* The page tables are mapped right after KLIMIT. For this reason,
             * the contents of the page table which maps addresses starting at
             * KLIMIT is identical to that of the page directory (first page
             * directory in PAE mode).
             * 
             * Let's remember the location of this page table and copy the
             * contents of the page directory there later when we are done
             * building it. */
            page_table_p0 = page_table;
        }
        else {
            /* clear page table */
            for(idy = 0; idy < page_table_entries; ++idy) {
                clear_pte( get_pte_with_offset(page_table, idy) );
            }
        }
	}
    
    /* clear remaining entries: user space page tables are allocated on demand */
    while(idx < page_table_entries) {
        clear_pte( get_pte_with_offset(page_directory, idx) );
		++idx;
	}
    
    /* copy page directory to page table which maps page tables */
    for(idx = 0; idx < page_table_entries; ++idx) {
        copy_pte(
            get_pte_with_offset(page_table_p0,  idx),
            get_pte_with_offset(page_directory, idx) );
    }
    
    /* map page directory */
    vm_map_early(page_directory_addr, (addr_t)page_directory, VM_FLAGS_PAGE_TABLE, page_directory);
    
    return page_directory;
}
