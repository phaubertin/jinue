#ifndef _JINUE_KERNEL_ALLOC_H
#define _JINUE_KERNEL_ALLOC_H

#include <jinue/pfalloc.h>
#include <jinue/types.h>


typedef struct {
	pfaddr_t  *ptr;
	uint32_t   count;
} pfcache_t;


extern bool use_pfalloc_early;

extern pfcache_t global_pfcache;


#define pfalloc() pfalloc_from(&global_pfcache)

#define pffree(p) pffree_to(&global_pfcache, (p))


addr_t pfalloc_early(void);

void init_pfcache(pfcache_t *pfcache, pfaddr_t *stack_page);

pfaddr_t pfalloc_from(pfcache_t *pfcache);

void pffree_to(pfcache_t *pfcache, pfaddr_t pf);

#endif
