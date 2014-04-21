#include <assert.h>
#include <stdint.h>
#include <pfalloc.h>
#include <vm_macros.h>
#include <vm_x86.h>
#include <vm.h>
#include <x86.h>
#include <jinue/page_tables.h>


struct __pte_t {
    uint32_t entry;
};


static unsigned int vm_x86_page_table_offset_of(addr_t addr) {
    return PAGE_TABLE_OFFSET_OF(addr);
}

static unsigned int vm_x86_global_page_table_offset_of(addr_t addr) {
    return GLOBAL_PAGE_TABLE_OFFSET_OF(addr);
}

static unsigned int vm_x86_page_directory_offset_of(addr_t addr) {
    return PAGE_DIRECTORY_OFFSET_OF(addr);
}

static pte_t *vm_x86_page_table_of(addr_t addr) {
    return PAGE_TABLE_OF(addr);
}

static pte_t *vm_x86_page_table_pte_of(addr_t addr) {
    return PAGE_TABLE_PTE_OF(addr);
}

static pte_t *vm_x86_get_pde(addr_t addr) {
    pte_t *page_directory;
    
    page_directory = (pte_t *)page_directory_addr;
    
	return get_pte_with_offset(page_directory, vm_x86_page_directory_offset_of(addr));
}

static pte_t *vm_x86_get_pte(addr_t addr) {
    pte_t *pde;
    
    /* get page directory entry */
	pde = vm_x86_get_pde(addr);
	
	/** ASSERTION: there is a page directory entry marked present for this address */
	assert(pde->entry & VM_FLAG_PRESENT);
    
    return PTE_OF(addr);
}

static void vm_x86_alloc_page_table(addr_t vaddr) {
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

void vm_x86_set_pointers(void) {
    vm_x86_set_pte_pointers();
    
    page_directory_addr         = (addr_t)get_pte_with_offset((pte_t *)PAGE_TABLES_ADDR, PAGE_TABLE_ENTRIES * PAGE_TABLE_ENTRIES);
    page_table_entries          = (size_t)PAGE_TABLE_ENTRIES;
    
    page_table_offset_of        = vm_x86_page_table_offset_of;
    global_page_table_offset_of = vm_x86_global_page_table_offset_of;
    page_directory_offset_of    = vm_x86_page_directory_offset_of;
    page_table_of               = vm_x86_page_table_of;
    get_pte                     = vm_x86_get_pte;
    get_pde                     = vm_x86_get_pde;
    alloc_page_table            = vm_x86_alloc_page_table;    
}
