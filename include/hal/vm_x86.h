#ifndef _JINUE_KERNEL_VM_X86_H_
#define _JINUE_KERNEL_VM_X86_H_

#include <jinue/types.h>


void vm_init_x86(void);

unsigned int page_table_offset_of_x86(addr_t addr);

unsigned int page_directory_offset_of_x86(addr_t addr);

pte_t *page_table_of_x86(addr_t addr);

pte_t *page_table_pte_of_x86(addr_t addr);

pte_t *get_pte_x86(addr_t addr);

pte_t *get_pde_x86(addr_t addr);

pte_t *get_pte_with_offset_x86(pte_t *pte, unsigned int offset);

void set_pte_x86(pte_t *pte, pfaddr_t paddr, int flags);

void set_pte_flags_x86(pte_t *pte, int flags);

int get_pte_flags_x86(pte_t *pte);

pfaddr_t get_pte_pfaddr_x86(pte_t *pte);

void clear_pte_x86(pte_t *pte);

#endif
