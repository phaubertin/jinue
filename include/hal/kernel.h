#ifndef JINUE_HAL_KERNEL_H
#define JINUE_HAL_KERNEL_H

#include <hal/startup.h>
#include <stddef.h>
#include <types.h>
#include <vm_alloc.h>


extern int in_kernel;

extern addr_t kernel_region_top;

extern vm_alloc_t *global_page_allocator;

#endif
