#ifndef JINUE_HAL_VM_PRIVATE_H
#define JINUE_HAL_VM_PRIVATE_H

/** This header file contains private definitions shared by hal/vm.c and
 * hal/vm_pae.c. There should be no reason to include it anywhere else. */

#include <jinue-common/vm.h>
#include <hal/vm.h>
#include <hal/vm_pae.h>
#include <stdbool.h>
#include <stdint.h>

/** number of entries in page table or page directory */
#define PAGE_TABLE_ENTRIES  (PAGE_SIZE / sizeof(pte_t))

/** bit mask for page table or page directory offset */
#define PAGE_TABLE_MASK (PAGE_TABLE_ENTRIES - 1)

/** page table entry offset of virtual (linear) address */
#define PAGE_TABLE_OFFSET_OF(x)     ( ((uint32_t)(x) / PAGE_SIZE) & PAGE_TABLE_MASK )

/** page directory entry offset of virtual (linear address) */
#define PAGE_DIRECTORY_OFFSET_OF(x) ( ((uint32_t)(x) / (PAGE_SIZE * PAGE_TABLE_ENTRIES)) & PAGE_TABLE_MASK )


extern pte_t *global_page_tables;

extern addr_space_t initial_addr_space;


extern size_t page_table_entries;

extern addr_space_t *(*create_addr_space)(addr_space_t *);

extern addr_space_t *(*create_initial_addr_space)(void);

extern void (*destroy_addr_space)(addr_space_t *);

/** page table entry offset of virtual (linear) address */
extern unsigned int (*page_table_offset_of)(addr_t);

extern unsigned int (*page_directory_offset_of)(addr_t);

extern pte_t *(*lookup_page_directory)(addr_space_t *, void *, bool);

extern pte_t *(*get_pte_with_offset)(pte_t *, unsigned int);

extern void (*set_pte)(pte_t *, pfaddr_t, int);

extern void (*set_pte_flags)(pte_t *, int);

extern int (*get_pte_flags)(pte_t *);

extern pfaddr_t (*get_pte_pfaddr)(pte_t *);

extern void (*clear_pte)(pte_t *);

extern void (*copy_pte)(pte_t *, pte_t *);


pfaddr_t vm_clone_page_directory(pfaddr_t template_pfaddr, unsigned int start_index);

pte_t *vm_allocate_page_directory(unsigned int start_index, bool first_pd);

void vm_destroy_page_directory(pfaddr_t pdpfaddr, unsigned int from_index, unsigned int to_index);

#endif
