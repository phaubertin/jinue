#ifndef _JINUE_KERNEL_KERNEL_H_
#define _JINUE_KERNEL_KERNEL_H_

#include <jinue/types.h>
#include <hal/startup.h>
#include <stddef.h>
#include <elf.h>
#include <vm_alloc.h>


#define kernel_start ((addr_t)__start)


extern addr_t kernel_top;

extern addr_t kernel_region_top;

extern size_t kernel_size;

extern addr_t kernel_stack;

extern elf_header_t proc_elf;

extern linker_defined_t kernel_end;

extern linker_defined_t proc_elf_end;

extern vm_alloc_t *global_page_allocator;

void kernel(void);

void kinit(void);

void idle(void);

#endif

