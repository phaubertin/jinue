#include <kernel.h>
#include <alloc.h>
#include <assert.h>
#include <vm.h>
#include <x86.h>


/** 
	Map a page frame (physical page) to a virtual memory page.
	@param vaddr virtual address of mapping
	@param paddr address of page frame
	@param flags flags used for mapping (see VM_FLAG_x constants in vm.h)
*/
void vm_map(addr_t vaddr, physaddr_t paddr, unsigned long flags) {
	pte_t *pte, *pde;
	addr_t page_table;
	int idx;
	
	/** ASSERTION: we assume vaddr is aligned on a page boundary */
	assert( PAGE_OFFSET_OF(vaddr) == 0 );
	
	/** ASSERTION: we assume paddr is aligned on a page boundary */
	assert( PAGE_OFFSET_OF(paddr) == 0 );
	
	/* get page directory entry */
	pde = PDE_OF(vaddr);
	
	/* check if page table must be created */
	if( !(*pde & VM_FLAG_PRESENT) ) {
		/** TODO: fix this once PAE is activated */
		/* allocate a new page table */
		page_table = (addr_t)(unsigned long)alloc_page();
		
		/* map page table in the region of memory reserved for that purpose */
		pte = PAGE_TABLE_PTE_OF(vaddr);
		*pte = (pte_t)page_table | VM_FLAGS_PAGE_TABLE | VM_FLAG_PRESENT;
		
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
	*pte = (pte_t)(unsigned long)paddr | flags | VM_FLAG_PRESENT;
	
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

void vm_change_flags(addr_t vaddr, unsigned long flags) {
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
	*pte = (pte_t)( (unsigned long)*pte & ~PAGE_MASK ) | flags | VM_FLAG_PRESENT;
	
	/* invalidate TLB entry for the affected page */
	invalidate_tlb(vaddr);	
}

void vm_map_early(addr_t vaddr, physaddr_t paddr, unsigned long flags, pte_t *page_directory) {
	pte_t *page_table;
	
	
	/** ASSERTION: we assume vaddr is aligned on a page boundary */
	assert( PAGE_OFFSET_OF(vaddr) == 0 );
	
	/** ASSERTION: we assume paddr is aligned on a page boundary */
	assert( PAGE_OFFSET_OF(paddr) == 0 );	
	
	page_table = (pte_t *)page_directory[ PAGE_DIRECTORY_OFFSET_OF(vaddr) ];
	page_table = (pte_t *)( (unsigned int)page_table & ~PAGE_MASK  );
	
	page_table[ PAGE_TABLE_OFFSET_OF(vaddr) ] = (pte_t)paddr | flags | VM_FLAG_PRESENT;
}
