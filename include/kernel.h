#ifndef _JINUE_KERNEL_KERNEL_H_
#define _JINUE_KERNEL_KERNEL_H_

#include <elf.h>
#include <startup.h>
#include <stddef.h>
#include <vm_alloc.h>
#include <jinue/types.h>


#define kernel_start ((addr_t)start)


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

