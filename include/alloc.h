#ifndef _JINUE_ALLOC_H
#define _JINUE_ALLOC_H

#include <kernel.h>

void alloc_init(void);
addr_t alloc(unsigned int pages);

#endif

