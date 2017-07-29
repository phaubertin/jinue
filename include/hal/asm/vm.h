#ifndef JINUE_HAL_ASM_VM_H
#define JINUE_HAL_ASM_VM_H

#include <hal/asm/x86.h>

/** page is present in memory */
#define VM_FLAG_PRESENT       X86_PTE_PRESENT

/** page is read only */
#define VM_FLAG_READ_ONLY     0

/** page is read/write accessible */
#define VM_FLAG_READ_WRITE    X86_PTE_READ_WRITE

/** kernel mode page */
#define VM_FLAG_KERNEL        X86_PTE_GLOBAL

/** user mode page */
#define VM_FLAG_USER          X86_PTE_USER

/** page was accessed (read) */
#define VM_FLAG_ACCESSED      X86_PTE_ACCESSED

/** page was written to */
#define VM_FLAG_DIRTY         X86_PTE_DIRTY

#endif
