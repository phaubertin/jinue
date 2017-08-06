#ifndef JINUE_HAL_VM_PAE_H
#define JINUE_HAL_VM_PAE_H

/** This header file contains declarations for the PAE functions defined in
 * hal/vm_pae.c. It is intended to be included by hal/vm.c and hal/vm_pae.c.
 * There should be no reason to include it anywhere else. */


void vm_pae_enable(void);

void vm_pae_boot_init(void);

void vm_pae_create_pdpt_cache(void);

#endif

