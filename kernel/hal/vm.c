#include <kernel.h>
#include <assert.h>
#include <pfalloc.h>
#include <printk.h>
#include <stdint.h>
#include <vm.h>
#include <vm_x86.h>
#include <x86.h>


addr_t page_directory_addr;

size_t page_table_entries;

/** page table entry offset of virtual (linear) address */
unsigned int (*page_table_offset_of)(addr_t);

unsigned int (*page_directory_offset_of)(addr_t);

pte_t *(*page_table_of)(addr_t);

pte_t *(*page_table_pte_of)(addr_t);

pte_t *(*get_pte)(addr_t);

pte_t *(*get_pde)(addr_t);

pte_t *(*get_pte_with_offset)(pte_t *, unsigned int);

void (*set_pte)(pte_t *, pfaddr_t, int);

void (*set_pte_flags)(pte_t *, int);

int (*get_pte_flags)(pte_t *);

pfaddr_t (*get_pte_pfaddr)(pte_t *);

void (*clear_pte)(pte_t *);


void vm_init(void) {
    if(cpu_features & CPU_FEATURE_PAE) {
		printk("Processor supports Physical Address Extension (PAE).\n");
        
        /** TODO: change me once PAE support is implemented */
        vm_init_x86();
    }
    else {
        vm_init_x86();
    }
}


/** 
	Map a page frame (physical page) to a virtual memory page.
	@param vaddr virtual address of mapping
	@param paddr address of page frame
	@param flags flags used for mapping (see VM_FLAG_x constants in vm.h)
*/
void vm_map(addr_t vaddr, pfaddr_t paddr, int flags) {
	pte_t *pte, *pde;
	pfaddr_t page_table;
	int idx;
	
	/** ASSERTION: we assume vaddr is aligned on a page boundary */
	assert( page_offset_of(vaddr) == 0 );
	
	/** ASSERTION: we assume paddr is aligned on a page boundary */
	assert( PFADDR_CHECK(paddr) );
	
	/** ASSERTION: we assume the page frame is below the 4GB limit */
	assert( PFADDR_CHECK_4GB(paddr) );
	
	/* get page directory entry */
	pde = get_pde(vaddr);
	
	/* check if page table must be created */
	if( ! (get_pte_flags(pde) & VM_FLAG_PRESENT) ) {
		/* allocate a new page table */
		page_table = pfalloc();
		
        /* map page table in the region of memory reserved for that purpose */
		pte = page_table_pte_of(vaddr);
        set_pte(pte, page_table, VM_FLAGS_PAGE_TABLE | VM_FLAG_PRESENT);
		
        /* obtain virtual address of new page table */
		pte = page_table_of(vaddr);
		
		/** TODO: check this */
        /* invalidate TLB entry for new page table */
		invalidate_tlb( (addr_t)pte );
		
        /* zero content of page table */
		for(idx = 0; idx < page_table_entries; ++idx) {
            clear_pte( get_pte_with_offset(pte, idx) );
		}
		
		/* link to page table from page directory */
		if(vaddr < (addr_t)PLIMIT) {
            set_pte(pde, page_table, VM_FLAG_KERNEL | VM_FLAG_READ_WRITE | VM_FLAG_PRESENT);
		}
		else {
            set_pte(pde, page_table, VM_FLAG_USER   | VM_FLAG_READ_WRITE | VM_FLAG_PRESENT);
		}		
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

pte_t *vm_create_addr_space_early(void) {
    unsigned int idx, idy;
    addr_t addr;
    pte_t *page_directory;
    pte_t *page_table;
    pte_t *pde;
    
    /* Allocate the first page directory. Since paging is not yet
	   enabled, virtual and physical addresses are the same.  */
	page_directory = (pte_t *)pfalloc_early();
	
	/* allocate page tables for kernel data/code region (0..PLIMIT) and add
	   relevant entries to page directory */
	for(idx = 0; idx < page_directory_offset_of(PLIMIT); ++idx) {
		page_table = (pte_t *)pfalloc_early();
        
        set_pte(
            get_pte_with_offset(page_directory, idx),
            PTR_TO_PFADDR(page_table),
            VM_FLAG_PRESENT | VM_FLAG_KERNEL | VM_FLAG_READ_WRITE );
		
        for(idy = 0; idy < page_table_entries; ++idy) {
            clear_pte( get_pte_with_offset(page_table, idy) );
		}
	}
	
    while(idx < page_table_entries) {
        clear_pte( get_pte_with_offset(page_directory, idx) );
		++idx;
	}

    /* map page directory */
	vm_map_early(page_directory_addr, (addr_t)page_directory, VM_FLAGS_PAGE_TABLE, page_directory);
		
	/* map page tables */
	for(idx = 0, addr = (addr_t)PAGE_TABLES_ADDR; idx < page_directory_offset_of(PLIMIT); ++idx, addr += PAGE_SIZE)	{
        pde = get_pte_with_offset(page_directory, idx);
        page_table = (pte_t *)(get_pte_pfaddr(pde) << PFADDR_SHIFT);
		
		vm_map_early((addr_t)addr, (addr_t)page_table, VM_FLAGS_PAGE_TABLE, page_directory);		
	}
    
    return page_directory;
}
