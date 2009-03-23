#ifndef _JINUE_VM_H_
#define _JINUE_VM_H_

/** number of bits in virtual address for offset inside page */
#define PAGE_BITS 12

/** size of page */
#define PAGE_SIZE (1<<PAGE_BITS) /* 4096 */

/** number of bits in virtual address for page table entry */
#define PAGE_TABLE_BITS 10

/** number of entries in page table */
#define PAGE_TABLE_ENTRIES (1<<PAGE_TABLE_BITS)

/** size of a page table */
#define PAGE_TABLE_SIZE PAGE_SIZE

/** size of a page table entry, in bytes */
#define PTE_SIZE 4

/** Virtual address range 0 to KLIMIT is reserved by kernel to store global
   data structures. 
   
   Kernel image must be completely inside this region. This region has the same
   mapping in the address space of all processes. Size must be a multiple of the
   size described by a single page directory entry (PTE_SIZE * PAGE_SIZE). */
#define KLIMIT (1<<24) /* 16M */

/** Virtual address range KLIMIT to PLIMIT is reserved by kernel to store data
   structures specific to the current process.
   
   The mapping of this region changes from one address space to the next. Size
   must be a multiple of the size described by a single page directory entry
   (PTE_SIZE * PAGE_SIZE). */
#define PLIMIT ( KLIMIT + (1<<24) ) /* 16M */

/** This is where the page tables are mapped in every address space. This
   requires a virtual memory region of size 4M, which must reside completely
   inside region spanning from KLIMIT to PLIMIT. Must be aligned on a 4M
   boundary */
#define PAGE_TABLES_ADDR KLIMIT

/** This is where the page directory is mapped in every address space. It must
   reside in region spanning from KLIMIT to PLIMIT. */
#define PAGE_DIRECTORY_ADDR (KLIMIT + PAGE_TABLE_ENTRIES * PAGE_TABLE_SIZE)

#endif

