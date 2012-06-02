#include <kernel.h>
#include <assert.h>
#include <pfalloc.h>
#include <stdint.h>
#include <vm.h>
#include <x86.h>


/** 
	Map a page frame (physical page) to a virtual memory page.
	@param vaddr virtual address of mapping
	@param paddr address of page frame
	@param flags flags used for mapping (see VM_FLAG_x constants in vm.h)
*/
void vm_map(addr_t vaddr, pfaddr_t paddr, uint32_t flags) {
	pte_t *pte, *pde;
	pte_t page_table;
	int idx;
	
	/** ASSERTION: we assume vaddr is aligned on a page boundary */
	assert( PAGE_OFFSET_OF(vaddr) == 0 );
	
	/** ASSERTION: we assume paddr is aligned on a page boundary */
	assert( PFADDR_CHECK(paddr) );
	
	/** ASSERTION: we assume the page frame is below the 4GB limit */
	assert( PFADDR_CHECK_4GB(paddr) );
	
	/* get page directory entry */
	pde = PDE_OF(vaddr);
	
	/* check if page table must be created */
	if( ! (*pde & VM_FLAG_PRESENT) ) {
		/** TODO: fix this once PAE is activated */
		/* allocate a new page table */
		page_table = (pte_t)pfalloc() << PFADDR_SHIFT;
		
		/* map page table in the region of memory reserved for that purpose */
		pte = PAGE_TABLE_PTE_OF(vaddr);
		*pte = page_table | VM_FLAGS_PAGE_TABLE | VM_FLAG_PRESENT;
		
		/* obtain virtual address of new page table */
		pte = PAGE_TABLE_OF(vaddr);
		
		/* invalidate TLB entry for new page table */
		invalidate_tlb( (addr_t)pte );
		
		/* zero content of page table */
		for(idx = 0; idx < PAGE_TABLE_ENTRIES; ++idx) {
			pte[idx] = 0;
		}
		
		/* link to page table from page directory */
		if(vaddr < (addr_t)PLIMIT) {
			*pde = (pte_t)page_table | VM_FLAG_KERNEL | VM_FLAG_READ_WRITE | VM_FLAG_PRESENT;
		}
		else {
			*pde = (pte_t)page_table | VM_FLAG_USER   | VM_FLAG_READ_WRITE | VM_FLAG_PRESENT;
		}		
	}
	
	/** TODO: fix this once PAE is activated */
	/* perform the actual mapping */
	pte = PTE_OF(vaddr);
	*pte = (pte_t)(paddr << PFADDR_SHIFT) | flags | VM_FLAG_PRESENT;
	
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
	assert( PAGE_OFFSET_OF(addr) == 0 );
	
	pte = PTE_OF(addr);
	*pte = NULL;
	
	invalidate_tlb(addr);
}

pfaddr_t vm_lookup_pfaddr(addr_t addr) {
	pte_t *pte;
	uint32_t paddr;
	
	/* get page frame base address from page tables */
	pte = PTE_OF(addr);
	paddr = (uint32_t)*pte & ~PAGE_MASK;
	
	return (paddr >> PFADDR_SHIFT);
}

void vm_change_flags(addr_t vaddr, uint32_t flags) {
	pte_t *pte, *pde;
	
	
	/** ASSERTION: we assume vaddr is aligned on a page boundary */
	assert( PAGE_OFFSET_OF(vaddr) == 0 );

	/* get page directory entry */
	pde = PDE_OF(vaddr);
	
	/** ASSERTION: there is a page directory entry marked present for this address */
	assert(*pde & VM_FLAG_PRESENT);
	
	/* get page table entry */
	pte = PTE_OF(vaddr);
	
	/** ASSERTION: there is a page table entry marked present for this address */
	assert(*pte & VM_FLAG_PRESENT);
	
	/* perform the flags change */
	*pte = (*pte & ~PAGE_MASK) | flags | VM_FLAG_PRESENT;
	
	/* invalidate TLB entry for the affected page */
	invalidate_tlb(vaddr);	
}

void vm_map_early(addr_t vaddr, addr_t paddr, uint32_t flags, pte_t *page_directory) {
	pte_t *page_table;
	
	
	/** ASSERTION: we assume vaddr is aligned on a page boundary */
	assert( PAGE_OFFSET_OF(vaddr) == 0 );
	
	/** ASSERTION: we assume paddr is aligned on a page boundary */
	assert( PAGE_OFFSET_OF(paddr) == 0 );
	
	
	page_table = (pte_t *)page_directory[ PAGE_DIRECTORY_OFFSET_OF(vaddr) ];
	page_table = (pte_t *)( (uint32_t)page_table & ~PAGE_MASK  );
	
	page_table[ PAGE_TABLE_OFFSET_OF(vaddr) ] = (pte_t)paddr | flags | VM_FLAG_PRESENT;
}
