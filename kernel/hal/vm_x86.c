#include <assert.h>
#include <stdint.h>
#include <pfalloc.h>
#include <vm_macros.h>
#include <vm_x86.h>
#include <vm.h>
#include <x86.h>


struct __pte_t {
    uint32_t entry;
};


void vm_x86_set_pointers(void) {
    page_directory_addr         = (addr_t)vm_x86_get_pte_with_offset((pte_t *)PAGE_TABLES_ADDR, PAGE_TABLE_ENTRIES * PAGE_TABLE_ENTRIES);
    page_table_entries          = (size_t)PAGE_TABLE_ENTRIES;
    
    page_table_offset_of        = vm_x86_page_table_offset_of;
    global_page_table_offset_of = vm_x86_global_page_table_offset_of;
    page_directory_offset_of    = vm_x86_page_directory_offset_of;
    page_table_of               = vm_x86_page_table_of;
    get_pte                     = vm_x86_get_pte;
    get_pde                     = vm_x86_get_pde;
    get_pte_with_offset         = vm_x86_get_pte_with_offset;
    set_pte                     = vm_x86_set_pte;
    set_pte_flags               = vm_x86_set_pte_flags;
    get_pte_flags               = vm_x86_get_pte_flags;
    get_pte_pfaddr              = vm_x86_get_pte_pfaddr;
    clear_pte                   = vm_x86_clear_pte;
    copy_pte                    = vm_x86_copy_pte;
    alloc_page_table            = vm_x86_alloc_page_table;
}

unsigned int vm_x86_page_table_offset_of(addr_t addr) {
    return PAGE_TABLE_OFFSET_OF(addr);
}

unsigned int vm_x86_global_page_table_offset_of(addr_t addr) {
    return GLOBAL_PAGE_TABLE_OFFSET_OF(addr);
}

unsigned int vm_x86_page_directory_offset_of(addr_t addr) {
    return PAGE_DIRECTORY_OFFSET_OF(addr);
}

pte_t *vm_x86_page_table_of(addr_t addr) {
    return PAGE_TABLE_OF(addr);
}

pte_t *vm_x86_page_table_pte_of(addr_t addr) {
    return PAGE_TABLE_PTE_OF(addr);
}

pte_t *vm_x86_get_pte(addr_t addr) {
    pte_t *pde;
    
    /* get page directory entry */
	pde = vm_x86_get_pde(addr);
	
	/** ASSERTION: there is a page directory entry marked present for this address */
	assert(pde->entry & VM_FLAG_PRESENT);
    
    return PTE_OF(addr);
}

pte_t *vm_x86_get_pde(addr_t addr) {
    pte_t *page_directory;
    
    page_directory = page_directory_addr;
    
	return vm_x86_get_pte_with_offset(page_directory, vm_x86_page_directory_offset_of(addr));
}

pte_t *vm_x86_get_pte_with_offset(pte_t *pte, unsigned int offset) {
    return &pte[offset];
}

void vm_x86_set_pte(pte_t *pte, pfaddr_t paddr, int flags) {
    pte->entry = ((uint32_t)paddr << PFADDR_SHIFT) | flags;
}

void vm_x86_set_pte_flags(pte_t *pte, int flags) {
    pte->entry = (pte->entry & ~PAGE_MASK) | flags;
}

int vm_x86_get_pte_flags(pte_t *pte) {
    return pte->entry & PAGE_MASK;
}

pfaddr_t vm_x86_get_pte_pfaddr(pte_t *pte) {
    return (pte->entry & ~PAGE_MASK) >> PFADDR_SHIFT;
}

void vm_x86_clear_pte(pte_t *pte) {
    pte->entry = 0;
}

void vm_x86_copy_pte(pte_t *dest, pte_t *src) {
    dest->entry = src->entry;
}

void vm_x86_alloc_page_table(addr_t vaddr) {
    pte_t *pte, *pde;
	pfaddr_t page_table;
	int idx;
    
    /* get page directory entry */
    pde = get_pde(vaddr);
    
    if( ! (get_pte_flags(pde) & VM_FLAG_PRESENT) ) {
        /* allocate a new page table */
        page_table = pfalloc();
        
        /* map page table in the region of memory reserved for that purpose */
        pte = vm_x86_page_table_pte_of(vaddr);
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
        set_pte(pde, page_table, VM_FLAG_USER | VM_FLAG_READ_WRITE | VM_FLAG_PRESENT);
    }
}
