#ifndef _JINUE_KERNEL_VM_MACROS_H_
#define _JINUE_KERNEL_VM_MACROS_H_

#include <vm.h>

/* ------ page tables ------ */

/** number of entries in page table or page directory */
#define PAGE_TABLE_ENTRIES  (PAGE_SIZE / sizeof(pte_t))

/** bit mask for page table or page directory offset */
#define PAGE_TABLE_MASK (PAGE_TABLE_ENTRIES - 1)

/** page table entry offset of virtual (linear) address */
#define PAGE_TABLE_OFFSET_OF(x)  ( ((uint32_t)(x) / PAGE_SIZE) & PAGE_TABLE_MASK )

/** page directory entry offset of virtual (linear address) */
#define PAGE_DIRECTORY_OFFSET_OF(x)  ((uint32_t)(x) / (PAGE_SIZE * PAGE_TABLE_ENTRIES))

/** This is where the page directory is mapped in every address space. It must
   reside in the region spanning from KLIMIT to PLIMIT. */
#define PAGE_DIRECTORY_ADDR (PAGE_TABLES_ADDR + PAGE_TABLE_ENTRIES * PAGE_TABLE_SIZE)

/* ------ virtual memory layout ------ */

/** low limit of region spanning from KLIMIT to PLIMIT actually available for
	mappings */
#define PMAPPING_START (PAGE_DIRECTORY_ADDR + PAGE_TABLE_SIZE)


/* ------ mapping of page tables in virtual memory ------ */

/** page directory in virtual memory */
#define PAGE_DIRECTORY ( (pte_t *)PAGE_DIRECTORY_ADDR )

/** page tables in virtual memory */
/** TODO: revert this once vm_x86.c is reverted also */
#define PAGE_TABLES ( (pte_t *)PAGE_TABLES_ADDR )

/** page table in virtual memory */
#define PAGE_TABLE_OF(x) ( &PAGE_TABLES[ PAGE_DIRECTORY_OFFSET_OF(x) * PAGE_TABLE_ENTRIES ] )

/** address of page directory entry in virtual memory */
#define PDE_OF(x) ( &PAGE_DIRECTORY[ PAGE_DIRECTORY_OFFSET_OF(x) ] )

/** address of page table entry in virtual memory */
#define PTE_OF(x) ( &PAGE_TABLE_OF(x)[ PAGE_TABLE_OFFSET_OF(x) ] )

/** page table which maps all page tables in memory */
#define PAGE_TABLES_TABLE ( PAGE_TABLE_OF( PAGE_TABLES_ADDR ) )

/** address of page entry in PAGE_OF_PAGE_TABLES */
#define PAGE_TABLE_PTE_OF(x) ( &PAGE_TABLES_TABLE[ PAGE_DIRECTORY_OFFSET_OF(x) ] )

#endif
