#ifndef _JINUE_COMMON_VM_H
#define _JINUE_COMMON_VM_H

#include <jinue-common/asm/vm.h>

#include <stdint.h>

/** byte offset in page of virtual (linear) address */
#define page_offset_of(x)   ((uintptr_t)(x) & PAGE_MASK)

/** sequential page number of virtual (linear) address */
#define page_number_of(x)   ((uintptr_t)(x) >> PAGE_SHIFT)

#endif
