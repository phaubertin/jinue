#ifndef JINUE_HAL_VM_PRIVATE_H
#define JINUE_HAL_VM_PRIVATE_H

#include <jinue-common/vm.h>
#include <stdint.h>

/** number of entries in page table or page directory */
#define PAGE_TABLE_ENTRIES  (PAGE_SIZE / sizeof(pte_t))

/** bit mask for page table or page directory offset */
#define PAGE_TABLE_MASK (PAGE_TABLE_ENTRIES - 1)

/** page table entry offset of virtual (linear) address */
#define PAGE_TABLE_OFFSET_OF(x)     ( ((uint32_t)(x) / PAGE_SIZE) & PAGE_TABLE_MASK )

/** page directory entry offset of virtual (linear address) */
#define PAGE_DIRECTORY_OFFSET_OF(x) ( ((uint32_t)(x) / (PAGE_SIZE * PAGE_TABLE_ENTRIES)) & PAGE_TABLE_MASK )


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

#endif
