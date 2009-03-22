#include <alloc.h>
#include <vm.h>

void vm_map(addr_t vaddr, addr_t paddr, unsigned long flags) {
	pte_t *pte;
	page_table_t *page_table;
	int idx;
	
	pte = PDE_OF(vaddr);
	
	/* check if page table must be created */
	if( !(*pte & VM_FLAG_PRESENT) ) {
		/* allocate a page for page table */
		page_table = (page_table_t *)alloc(PAGE_SIZE);
		
		/* link to page table from page directory */
		*pte = (pte_t)page_table | VM_FLAG_PRESENT;
		
		/* map page table in the region of memory reserved for that purpose */
		pte = PTE_OF_PAGE_TABLE_OF(vaddr);
		*pte = (pte_t)page_table | VM_FLAG_PRESENT;
		
		/* zero content of page table */
		for(idx = 0; idx < PT_ENTRIES; ++idx) {
			(*page_table)[idx] = 0;
		}
	}
	
	/* perform the actual mapping */
	pte = PTE_OF(vaddr);
	*pte = (pte_t)paddr | VM_FLAG_PRESENT;
}

void vm_unmap(addr_t addr) {
	pte_t *pte;
	
	pte = PTE_OF(addr);
	*pte = 0;
}

