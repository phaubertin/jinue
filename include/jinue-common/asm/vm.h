#ifndef _JINUE_ASM_VM_H
#define _JINUE_ASM_VM_H

#include <jinue-common/asm/types.h>


/** number of bits in virtual address for offset inside page */
#define PAGE_BITS           12

/** size of page */
#define PAGE_SIZE           (1<<PAGE_BITS) /* 4096 */

/** bit mask for offset in page */
#define PAGE_MASK           (PAGE_SIZE - 1)

/** The virtual address range starting at KLIMIT is reserved by the kernel. The
    region above KLIMIT has the same mapping in all address spaces.
    KLIMIT must be aligned on a 4MB boundary. */
#define KLIMIT              0xE0000000

/** stack base address (stack top) */
#define STACK_BASE          KLIMIT

/** initial stack size */
#define STACK_SIZE          (8 * PAGE_SIZE)

/** initial stack lower address */
#define STACK_START         (STACK_BASE - STACK_SIZE)

#endif
