#ifndef _JINUE_KERNEL_KERNEL_H_
#define _JINUE_KERNEL_KERNEL_H_

#include <startup.h>
#include <stddef.h>
#include <elf.h>

typedef void *addr_t;

typedef unsigned long long physaddr_t;

typedef unsigned long long physsize_t;

typedef unsigned long count_t;

#define kernel_start ((addr_t)start)

extern addr_t kernel_top;

extern addr_t kernel_region_top;

extern size_t kernel_size;

extern addr_t kernel_end;

extern addr_t kernel_stack;

extern elf_header_t proc_elf;

extern addr_t proc_elf_end;

void kernel(void);
void kinit(void);
void idle(void);

#endif

