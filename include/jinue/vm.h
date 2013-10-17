#ifndef _JINUE_VM_H_
#define _JINUE_VM_H_

#include <jinue/types.h>

/** number of bits in virtual address for offset inside page */
#define PAGE_SHIFT 12

/** size of page */
#define PAGE_SIZE (1<<PAGE_SHIFT) /* 4096 */

/** Virtual address range 0 to KLIMIT is reserved by kernel to store global
   data structures. 
   
   Kernel image must be completely inside this region. This region has the same
   mapping in the address space of all processes. Size must be a multiple of the
   size described by a single page directory entry. */
#define KLIMIT ((addr_t)(96 * MB)) /* 96M */

/** Virtual address range KLIMIT to PLIMIT is reserved by kernel to store data
   structures specific to the current process.
   
   The mapping of this region changes from one address space to the next. Size
   must be a multiple of the size described by a single page directory entry */
#define PLIMIT ((addr_t)( KLIMIT + 32 * MB)) /* 32M */

/** This is where the page tables are mapped in every address space. This
   requires a virtual memory region of size 4M, which must reside completely
   inside region spanning from KLIMIT to PLIMIT. Must be aligned on a 4M
   boundary */
#define PAGE_TABLES_ADDR KLIMIT

#endif
