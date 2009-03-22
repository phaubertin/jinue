#ifndef _JINUE_KERNEL_VM_H_
#define _JINUE_KERNEL_VM_H_

#include <kernel.h>
#include <jinue/vm.h>

/* number of bits in virtual address for page table entry */
#define PT_BITS 10

/* number of entries in page table */
#define PT_ENTRIES (1<<PT_BITS)

/* bit mask for page table entry */
#define PT_MASK (PT_ENTRIES - 1)

/* size of a page table */
#define PT_SIZE PAGE_SIZE

/* size of a page table entry, in bytes */
#define PTE_SIZE 4

/* number of bits in virtual address for page directory entry */
#define PD_BITS 10

/* number of entries in page directory */
#define PD_ENTRIES (1<<PD_BITS)

/* bit mask for page directory entry */
#define PD_MASK (PD_ENTRIES - 1)

/* size of page directory */
#define PD_SIZE PAGE_SIZE

/* size of a page directory entry, in bytes */
#define PDE_SIZE 4

/* page table entry of virtual address */
#define PT_OFFSET_OF(x)  ( ((unsigned long)(x) >> PAGE_BITS) & PT_MASK )

/* page directory entry of virtual address */
#define PD_OFFSET_OF(x)  ( ((unsigned long)(x) >> (PAGE_BITS + PT_BITS)) & PD_MASK )

/* This is where the page tables are mapped in every address space. This
   requires a virtual memory region of size 4M, which must reside completely
   inside region spanning from KLIMIT to PLIMIT. */
#define PAGE_TABLES KLIMIT

/* This is where the page directory is mapped in every address space. It must
   reside in region spanning from KLIMIT to PLIMIT. */
#define PAGE_DIRECTORY (KLIMIT + PD_ENTRIES * PT_SIZE)

/* limits of region spanning from KLIMIT to PLIMIT actually available for
   mappings */
#define PMAPPING_START (PAGE_DIRECTORY + PD_SIZE)
#define PMAPPING_END   PLIMIT

/* address of page directory entry */
#define PDE_OF(x) ( (pte_t *)(PAGE_DIRECTORY + PDE_SIZE * PD_OFFSET_OF(x)) )

/* address of page table entry */
#define PTE_OF(x) ( (pte_t *)(PAGE_TABLES + PT_SIZE * PD_OFFSET_OF(x) + PTE_SIZE * PT_OFFSET_OF(x)) )

/* page table entry for mapping a page table in virtual memory */
#define PTE_OF_PAGE_TABLE_OF(x) ( (pte_t *)(PAGE_TABLES + PT_SIZE * PD_OFFSET_OF(PAGE_TABLES) + PTE_SIZE * PD_OFFSET_OF(x)) )

/* flags */
#define VM_FLAG_KERNEL        0
#define VM_FLAG_PRESENT       (1<< 0)
#define VM_FLAG_READ_ONLY     (1<< 1)
#define VM_FLAG_USER          (1<< 2)
#define VM_FLAG_WRITE_THROUGH (1<< 3)
#define VM_FLAG_CACHE_DISABLE (1<< 4)
#define VM_FLAG_ACCESSED      (1<< 5)
#define VM_FLAG_DIRTY         (1<< 6)
#define VM_FLAG_BIG_PAGE      (1<< 7)
#define VM_FLAG_GLOBAL        (1<< 8)


typedef unsigned long pte_t;
typedef pte_t page_table_t[PT_ENTRIES];

void vm_map(addr_t vaddr, addr_t paddr, unsigned long flags);
void vm_unmap(addr_t addr);

#endif

