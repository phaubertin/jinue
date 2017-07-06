#ifndef _JINUE_VM_H_
#define _JINUE_VM_H_

#include <jinue/asm/vm.h>

#include <jinue/types.h>
#include <stdint.h>

/** byte offset in page of virtual (linear) address */
#define page_offset_of(x)   ((uintptr_t)(x) & PAGE_MASK)

/** sequential page number of virtual (linear) address */
#define page_number_of(x)   ((uintptr_t)(x) >> PAGE_SHIFT)

#endif
