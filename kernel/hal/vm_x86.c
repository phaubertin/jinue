#include <hal/vm_macros.h>
#include <hal/vm_x86.h>
#include <hal/vm.h>
#include <hal/x86.h>
#include <assert.h>
#include <stdint.h>
#include <pfalloc.h>


struct __pte_t {
    uint32_t entry;
};


static unsigned int vm_x86_page_table_offset_of(addr_t addr) {
    return PAGE_TABLE_OFFSET_OF(addr);
}

static unsigned int vm_x86_page_directory_offset_of(addr_t addr) {
    return PAGE_DIRECTORY_OFFSET_OF(addr);
}

static pte_t *vm_x86_get_pte_with_offset(pte_t *pte, unsigned int offset) {
    return &pte[offset];
}

static void vm_x86_set_pte(pte_t *pte, pfaddr_t paddr, int flags) {
    /** TODO: check paddr for 4GB limit */
    pte->entry = ((uint32_t)paddr << PFADDR_SHIFT) | flags;
}

static void vm_x86_set_pte_flags(pte_t *pte, int flags) {
    pte->entry = (pte->entry & ~PAGE_MASK) | flags;
}

static int vm_x86_get_pte_flags(pte_t *pte) {
    return pte->entry & PAGE_MASK;
}

static pfaddr_t vm_x86_get_pte_pfaddr(pte_t *pte) {
    return (pte->entry & ~PAGE_MASK) >> PFADDR_SHIFT;
}

static void vm_x86_clear_pte(pte_t *pte) {
    pte->entry = 0;
}

static void vm_x86_copy_pte(pte_t *dest, pte_t *src) {
    dest->entry = src->entry;
}

size_t page_table_entries                               = (size_t)PAGE_TABLE_ENTRIES;

/** page table entry offset of virtual (linear) address */
unsigned int (*page_table_offset_of)(addr_t)            = vm_x86_page_table_offset_of;

unsigned int (*page_directory_offset_of)(addr_t)        = vm_x86_page_directory_offset_of;

pte_t *(*get_pte_with_offset)(pte_t *, unsigned int)    = vm_x86_get_pte_with_offset;

void (*set_pte)(pte_t *, pfaddr_t, int)                 = vm_x86_set_pte;

void (*set_pte_flags)(pte_t *, int)                     = vm_x86_set_pte_flags;

int (*get_pte_flags)(pte_t *)                           = vm_x86_get_pte_flags;

pfaddr_t (*get_pte_pfaddr)(pte_t *)                     = vm_x86_get_pte_pfaddr;

void (*clear_pte)(pte_t *)                              = vm_x86_clear_pte;

void (*copy_pte)(pte_t *, pte_t *)                      = vm_x86_copy_pte;
