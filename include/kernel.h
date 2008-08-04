#ifndef _JINUE_KERNEL_H_
#define _JINUE_KERNEL_H_

#include <startup.h>
#include <stddef.h>

typedef void *addr_t;

#define kernel_start ((addr_t)start)
extern addr_t kernel_top;
extern size_t kernel_size;

void kernel(void);
void kinit(void);
void idle(void);

#endif

