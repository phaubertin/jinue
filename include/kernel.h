#ifndef _JINUE_KERNEL_KERNEL_H_
#define _JINUE_KERNEL_KERNEL_H_

#include <startup.h>
#include <stddef.h>

typedef void *addr_t;

typedef unsigned long long physaddr_t;

typedef unsigned long long physsize_t;

typedef unsigned long count_t;

#define kernel_start ((addr_t)start)
extern addr_t kernel_top;
extern addr_t kernel_region_top;
extern size_t kernel_size;
extern addr_t kernel_stack;
extern unsigned long proc_elf;

void kernel(void);
void kinit(void);
void idle(void);

#endif

