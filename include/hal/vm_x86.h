#ifndef _JINUE_KERNEL_VM_X86_H_
#define _JINUE_KERNEL_VM_X86_H_

#include <jinue/types.h>


void vm_x86_set_pointers(void);

unsigned int vm_x86_page_table_offset_of(addr_t addr);

unsigned int vm_x86_global_page_table_offset_of(addr_t addr);

unsigned int vm_x86_page_directory_offset_of(addr_t addr);

pte_t *vm_x86_page_table_of(addr_t addr);

pte_t *vm_x86_page_table_pte_of(addr_t addr);

pte_t *vm_x86_get_pte(addr_t addr);

pte_t *vm_x86_get_pde(addr_t addr);

pte_t *vm_x86_get_pte_with_offset(pte_t *pte, unsigned int offset);

void vm_x86_set_pte(pte_t *pte, pfaddr_t paddr, int flags);

void vm_x86_set_pte_flags(pte_t *pte, int flags);

int vm_x86_get_pte_flags(pte_t *pte);

pfaddr_t vm_x86_get_pte_pfaddr(pte_t *pte);

void vm_x86_clear_pte(pte_t *pte);

void vm_x86_copy_pte(pte_t *dest, pte_t *src);

void vm_x86_alloc_page_table(addr_t vaddr);

#endif
