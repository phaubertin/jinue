#ifndef JINUE_HAL_VM_H
#define JINUE_HAL_VM_H

/** This header file contains the public interface of the low-level page table
 * management code located in hal/vm.c and hal/vm_pae.c. */

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

#define ADDR_4GB    UINT64_C(0x100000000)


void vm_boot_init(void);

void vm_map_kernel(addr_t vaddr, pfaddr_t paddr, int flags);

void vm_map_user(addr_space_t *addr_space, addr_t vaddr, pfaddr_t paddr, int flags);

void vm_unmap_kernel(addr_t addr);

void vm_unmap_user(addr_space_t *addr_space, addr_t addr);

pfaddr_t vm_lookup_pfaddr(addr_space_t *addr_space, addr_t addr);

void vm_change_flags(addr_space_t *addr_space, addr_t addr, int flags);

void vm_map_early(addr_t vaddr, pfaddr_t paddr, int flags);

addr_space_t *vm_create_addr_space(addr_space_t *addr_space);

addr_space_t *vm_create_initial_addr_space(void);

void vm_destroy_addr_space(addr_space_t *addr_space);

void vm_switch_addr_space(addr_space_t *addr_space);

#endif

