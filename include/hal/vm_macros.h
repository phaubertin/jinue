#ifndef _JINUE_KERNEL_VM_MACROS_H_
#define _JINUE_KERNEL_VM_MACROS_H_

#include <jinue/vm.h>
#include <stdint.h>

/* ------ page tables ------ */

/** number of entries in page table or page directory */
#define PAGE_TABLE_ENTRIES  (PAGE_SIZE / sizeof(pte_t))

/** bit mask for page table or page directory offset */
#define PAGE_TABLE_MASK (PAGE_TABLE_ENTRIES - 1)

/** page table entry offset of virtual (linear) address */
#define PAGE_TABLE_OFFSET_OF(x)     ( ((uint32_t)(x) / PAGE_SIZE) & PAGE_TABLE_MASK )

/** page directory entry offset of virtual (linear address) */
#define PAGE_DIRECTORY_OFFSET_OF(x) ( ((uint32_t)(x) / (PAGE_SIZE * PAGE_TABLE_ENTRIES)) & PAGE_TABLE_MASK )

#endif
