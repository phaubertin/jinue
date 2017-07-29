#ifndef JINUE_HAL_VM_H
#define JINUE_HAL_VM_H

#include <hal/asm/vm.h>

#include <jinue-common/vm.h>
#include <hal/pfaddr.h>
#include <hal/types.h>

/** convert a physical address to a virtual address before the switch to the first address space */
#define EARLY_PHYS_TO_VIRT(x)   (((uintptr_t)(x)) + KLIMIT)

/** convert a virtual address to a physical address before the switch to the first address space */
#define EARLY_VIRT_TO_PHYS(x)   (((uintptr_t)(x)) - KLIMIT)

/** convert a pointer to a page frame address (early mappings) */
#define EARLY_PTR_TO_PFADDR(x)  ( (pfaddr_t)( (EARLY_VIRT_TO_PHYS(x) >> PFADDR_SHIFT) ) )


extern pte_t *global_page_tables;

extern addr_space_t initial_addr_space;

extern size_t page_table_entries;

/** page table entry offset of virtual (linear) address */
extern unsigned int (*page_table_offset_of)(addr_t);

extern unsigned int (*page_directory_offset_of)(addr_t);

extern pte_t *(*get_pte_with_offset)(pte_t *, unsigned int);

extern void (*set_pte)(pte_t *, pfaddr_t, int);

extern void (*set_pte_flags)(pte_t *, int);

extern int (*get_pte_flags)(pte_t *);

extern pfaddr_t (*get_pte_pfaddr)(pte_t *);

extern void (*clear_pte)(pte_t *);

extern void (*copy_pte)(pte_t *, pte_t *);


void vm_boot_init(void);

void vm_map(addr_space_t *addr_space, addr_t vaddr, pfaddr_t paddr, int flags);

void vm_unmap(addr_space_t *addr_space, addr_t addr);

pfaddr_t vm_lookup_pfaddr(addr_space_t *addr_space, addr_t addr);

void vm_change_flags(addr_space_t *addr_space, addr_t addr, int flags);

void vm_map_early(addr_t vaddr, pfaddr_t paddr, int flags);

addr_space_t *vm_create_addr_space(addr_space_t *addr_space);

addr_space_t *vm_create_initial_addr_space(void);

void vm_destroy_addr_space(addr_space_t *addr_space);

void vm_switch_addr_space(addr_space_t *addr_space);


#define vm_map_global(vaddr, paddr, flags) \
    vm_map(NULL, vaddr, paddr, flags)

#define vm_unmap_global(addr) \
    vm_unmap(NULL, addr)

#endif

