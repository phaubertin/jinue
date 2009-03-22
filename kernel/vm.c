#include <kernel.h>
#include <alloc.h>
#include <vm.h>

void vm_map(addr_t vaddr, addr_t paddr, unsigned long flags) {
	pte_t *pte;
	addr_t page_table;
	int idx;
	
	pte = PDE_OF(vaddr);
	
	/* check if page table must be created */
	if( !(*pte & VM_FLAG_PRESENT) ) {
		/* allocate a page for page table */
		page_table = alloc(PAGE_SIZE);
		
		/* link to page table from page directory */
		*pte = (pte_t)page_table | VM_FLAG_PRESENT;
		
		/* map page table in the region of memory reserved for that purpose */
		pte = PAGE_TABLE_PTE_OF(vaddr);
		*pte = (pte_t)page_table | VM_FLAG_PRESENT;
		
		/* obtain virtual address of new page table */
		pte = PAGE_TABLE_OF(vaddr);
		
		/* zero content of page table */
		for(idx = 0; idx < PAGE_TABLE_ENTRIES; ++idx) {
			pte[idx] = 0;
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

