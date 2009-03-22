#ifndef _JINUE_VM_H_
#define _JINUE_VM_H_

/* number of bits in virtual address for offset inside page */
#define PAGE_BITS 12

/* size of page */
#define PAGE_SIZE (1<<PAGE_BITS) /* 4096 */

/* bit mask for offset in page */
#define PAGE_MASK (PAGE_SIZE - 1)

/* offset in page of virtual address */
#define PAGE_OFFSET_OF(x)  ((unsigned long)(x) & PAGE_MASK)

/* Virtual address range 0 to KLIMIT is reserved by kernel to store global
   data structures. 
   
   Kernel image must be completely inside this region. This region has the same
   mapping in the address space of all processes. Size must be a multiple of the
   size described by a single page directory entry (PTE_SIZE * PAGE_SIZE). */
#define KLIMIT (1<<24) /* 16M */

/* Virtual address range KLIMIT to PLIMIT is reserved by kernel to store data
   structures specific to the current process.
   
   The mapping of this region changes from one address space to the next. Size
   must be a multiple of the size described by a single page directory entry
   (PTE_SIZE * PAGE_SIZE). */
#define PLIMIT ( KLIMIT + (1<<24) ) /* 16M */

#endif

