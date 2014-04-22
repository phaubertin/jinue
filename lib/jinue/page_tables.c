#include <jinue/page_tables.h>

struct __pte_t {
    uint32_t entry;
};

pte_t *(*get_pte_with_offset)(pte_t *, unsigned int);

void (*set_pte)(pte_t *, pfaddr_t, int);

void (*set_pte_flags)(pte_t *, int);

int (*get_pte_flags)(pte_t *);

pfaddr_t (*get_pte_pfaddr)(pte_t *);

void (*clear_pte)(pte_t *);

void (*copy_pte)(pte_t *, pte_t *);


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

void vm_x86_set_pte_pointers(void) {
	get_pte_with_offset         = vm_x86_get_pte_with_offset;
    set_pte                     = vm_x86_set_pte;
    set_pte_flags               = vm_x86_set_pte_flags;
    get_pte_flags               = vm_x86_get_pte_flags;
    get_pte_pfaddr              = vm_x86_get_pte_pfaddr;
    clear_pte                   = vm_x86_clear_pte;
    copy_pte                    = vm_x86_copy_pte;    
}
