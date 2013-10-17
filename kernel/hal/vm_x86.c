#include <assert.h>
#include <stdint.h>
#include <vm_macros.h>
#include <vm_x86.h>
#include <vm.h>

/** number of bits in virtual address for page table or page directory offset */
#define PAGE_TABLE_BITS 10


struct __pte_t {
    uint32_t entry;
};


void vm_init_x86(void) {
    page_directory_addr         = (addr_t)PAGE_DIRECTORY_ADDR;
    page_table_entries          = (size_t)PAGE_TABLE_ENTRIES;
    
    page_table_offset_of        = page_table_offset_of_x86;
    page_directory_offset_of    = page_directory_offset_of_x86;
    page_table_of               = page_table_of_x86;
    page_table_pte_of           = page_table_pte_of_x86;
    get_pte                     = get_pte_x86;
    get_pde                     = get_pde_x86;
    get_pte_with_offset         = get_pte_with_offset_x86;
    set_pte                     = set_pte_x86;
    set_pte_flags               = set_pte_flags_x86;
    get_pte_flags               = get_pte_flags_x86;
    get_pte_pfaddr              = get_pte_pfaddr_x86;
    clear_pte                   = clear_pte_x86;
}

unsigned int page_table_offset_of_x86(addr_t addr) {
    return PAGE_TABLE_OFFSET_OF(addr);
}

unsigned int page_directory_offset_of_x86(addr_t addr) {
    return PAGE_DIRECTORY_OFFSET_OF(addr);
}

pte_t *page_table_of_x86(addr_t addr) {
    return PAGE_TABLE_OF(addr);
}

pte_t *page_table_pte_of_x86(addr_t addr) {
    return PAGE_TABLE_PTE_OF(addr);
}

addr_t page_directory_addr_x86(void) {
    return PAGE_DIRECTORY_ADDR;
}

pte_t *get_pte_x86(addr_t addr) {
    pte_t *pde;
    
    /* get page directory entry */
	pde = PDE_OF(addr);
	
	/** ASSERTION: there is a page directory entry marked present for this address */
	assert(pde->entry & VM_FLAG_PRESENT);
    
    return PTE_OF(addr);
}

pte_t *get_pde_x86(addr_t addr) {
	return PDE_OF(addr);
}

pte_t *get_pte_with_offset_x86(pte_t *pte, unsigned int offset) {
    return &pte[offset];
}

void set_pte_x86(pte_t *pte, pfaddr_t paddr, int flags) {
    pte->entry = ((uint32_t)paddr << PFADDR_SHIFT) | flags;
}

void set_pte_flags_x86(pte_t *pte, int flags) {
    pte->entry = (pte->entry & ~PAGE_MASK) | flags;
}

int get_pte_flags_x86(pte_t *pte) {
    return pte->entry & PAGE_MASK;
}

pfaddr_t get_pte_pfaddr_x86(pte_t *pte) {
    return (pte->entry & ~PAGE_MASK) >> PFADDR_SHIFT;
}

void clear_pte_x86(pte_t *pte) {
    pte->entry = 0;
}
