#ifndef _JINUE_KERNEL_VM_H_
#define _JINUE_KERNEL_VM_H_

#include <kernel.h>
#include <jinue/vm.h>


/* ------ page offset ------ */

/** bit mask for offset in page */
#define PAGE_MASK (PAGE_SIZE - 1)

/** offset in page of virtual address */
#define PAGE_OFFSET_OF(x)  ((unsigned long)(x) & PAGE_MASK)


/* ------ page tables ------ */

/** type of a page table (or page directory) entry */
typedef unsigned long pte_t;

/** bit mask for page table entry */
#define PAGE_TABLE_MASK (PAGE_TABLE_ENTRIES - 1)

/** page table entry offset of virtual (linear) address */
#define PAGE_TABLE_OFFSET_OF(x)  ( ((unsigned long)(x) >> PAGE_BITS) & PAGE_TABLE_MASK )

/** page directory entry offset of virtual (linear address) */
#define PAGE_DIRECTORY_OFFSET_OF(x)  ((unsigned long)(x) >> (PAGE_BITS + PAGE_TABLE_BITS))

/** type of a page table */
typedef pte_t page_table_t[PAGE_TABLE_ENTRIES];


/* ------ virtual memory layout ------ */

/** low limit of region spanning from KLIMIT to PLIMIT actually available for
	mappings */
#define PMAPPING_START (PAGE_DIRECTORY_ADDR + PAGE_TABLE_SIZE)

/** high limit of region spanning from KLIMIT to PLIMIT actually available for
	mappings */
#define PMAPPING_END   PLIMIT


/* ------ mapping of page tables in virtual memory ------ */

/** page directory in virtual memory */
#define PAGE_DIRECTORY ( (pte_t *)PAGE_DIRECTORY_ADDR )

/** page tables in virtual memory */
#define PAGE_TABLES ( (page_table_t *)PAGE_TABLES_ADDR )

/** page table in virtual memory */
#define PAGE_TABLE_OF(x) ( PAGE_TABLES[ PAGE_DIRECTORY_OFFSET_OF(x) ] )

/** address of page directory entry in virtual memory */
#define PDE_OF(x) ( &PAGE_DIRECTORY[ PAGE_DIRECTORY_OFFSET_OF(x) ] )

/** address of page table entry in virtual memory */
#define PTE_OF(x) ( &PAGE_TABLE_OF(x)[ PAGE_TABLE_OFFSET_OF(x) ] )

/** page table which maps all page tables in memory */
#define PAGE_TABLES_TABLE ( PAGE_TABLE_OF( PAGE_TABLES_ADDR ) )

/** address of page entry in PAGE_OF_PAGE_TABLES */
#define PAGE_TABLE_PTE_OF(x) ( &PAGE_TABLES_TABLE[ PAGE_DIRECTORY_OFFSET_OF(x) ] )


/* ------ flags for page attributes ------ */

/** page is present in memory */
#define VM_FLAG_PRESENT       (1<< 0)

/** page is read only */
#define VM_FLAG_READ_ONLY     (1<< 1)

/** kernel mode page (default) */
#define VM_FLAG_KERNEL        0

/** user mode page */
#define VM_FLAG_USER          (1<< 2)

/** write-through cache policy for page */
#define VM_FLAG_WRITE_THROUGH (1<< 3)

/** uncached page */
#define VM_FLAG_CACHE_DISABLE (1<< 4)

/** page was accessed (read) */
#define VM_FLAG_ACCESSED      (1<< 5)

/** page was written to */
#define VM_FLAG_DIRTY         (1<< 6)

/** page directory entry describes a 4M page */
#define VM_FLAG_BIG_PAGE      (1<< 7)

/** page is global (mapped in every address space) */
#define VM_FLAG_GLOBAL        (1<< 8)

/** set of flags for a page table (or page directory) */
#define WM_FLAGS_PAGE_TABLE (VM_FLAG_USER | VM_FLAG_READ_ONLY)


void vm_map(addr_t vaddr, addr_t paddr, unsigned long flags);
void vm_unmap(addr_t addr);

#endif

