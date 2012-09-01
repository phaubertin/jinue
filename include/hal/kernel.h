#ifndef _JINUE_KERNEL_KERNEL_H_
#define _JINUE_KERNEL_KERNEL_H_

#include <jinue/types.h>
#include <hal/startup.h>
#include <stddef.h>
#include <vm_alloc.h>


#define kernel_start ((addr_t)__start)


extern addr_t kernel_top;

extern addr_t kernel_region_top;

extern size_t kernel_size;

extern addr_t kernel_stack;

extern linker_defined_t kernel_end;

extern vm_alloc_t *global_page_allocator;


void hal_start(void);

#endif

