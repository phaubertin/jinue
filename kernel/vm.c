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
void vm_map(addr_t vaddr, addr_t paddr, unsigned long flags) {
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
		/* allocate a new page table */
		page_table = alloc(PAGE_SIZE);
		
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
		*pde = (pte_t)page_table | VM_FLAG_USER | VM_FLAG_PRESENT;		
	}
	
	/* perform the actual mapping */
	pte = PTE_OF(vaddr);
	*pte = (pte_t)paddr | flags | VM_FLAG_PRESENT;
	
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
	*pte = 0;
	
	invalidate_tlb(addr);
}

