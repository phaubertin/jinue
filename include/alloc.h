#ifndef _JINUE_KERNEL_ALLOC_H
#define _JINUE_KERNEL_ALLOC_H

#include <kernel.h>
#include <stdbool.h>

#define alloc_page() ( (*__alloc_page)() )

typedef physaddr_t (*alloc_page_t)(void);

extern alloc_page_t __alloc_page;

extern bool use_alloc_page_early;


addr_t alloc_page_early(void);

physaddr_t do_not_call(void);

#endif
