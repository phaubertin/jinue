#ifndef _JINUE_ASM_VM_H
#define _JINUE_ASM_VM_H

/** number of bits in virtual address for offset inside page */
#define PAGE_SHIFT 12

/** size of page */
#define PAGE_SIZE           (1<<PAGE_SHIFT) /* 4096 */

/** bit mask for offset in page */
#define PAGE_MASK           (PAGE_SIZE - 1)

/** Virtual address range 0 to KLIMIT is reserved by kernel to store global
   data structures.

   Kernel image must be completely inside this region. This region has the same
   mapping in the address space of all processes. Size must be a multiple of the
   size described by a single page directory entry. */
#define KLIMIT ((addr_t)(128 * MB)) /* 128MB */

/** stack base address (stack top) */
#define STACK_BASE      (0 - PAGE_SIZE)

/** initial stack size */
#define STACK_SIZE      (8 * PAGE_SIZE)

/** initial stack lower address */
#define STACK_START     (STACK_BASE - STACK_SIZE)

#endif
