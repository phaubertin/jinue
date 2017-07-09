#ifndef JINUE_HAL_KERNEL_H
#define JINUE_HAL_KERNEL_H

#include <jinue/types.h>
#include <hal/startup.h>
#include <stddef.h>
#include <vm_alloc.h>

#include <hal/asm/kernel.h>


extern int in_kernel;

extern addr_t kernel_top;

extern addr_t kernel_region_top;

extern size_t kernel_size;

extern addr_t kernel_stack;

/* These next two symbols are defined by a linker script */
extern int kernel_start;

extern int kernel_end;

extern vm_alloc_t *global_page_allocator;

void hal_start(void);

#endif

