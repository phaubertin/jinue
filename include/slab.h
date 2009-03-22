#ifndef _JINUE_SLAB_H
#define _JINUE_SLAB_H

#include <vm_alloc.h> /* includes kernel.h */

struct slab_header_t {
	count_t    available;
	addr_t     free_list;
	struct slab_header_t *next;
	struct slab_header_t *prev;	
};

typedef struct slab_header_t slab_header_t;

struct slab_cache_t {
	size_t  obj_size;
	count_t per_slab;
	slab_header_t *empty;
	slab_header_t *partial;
	slab_header_t *full;
	struct vm_alloc_t *vm_allocator;
};

typedef struct slab_cache_t slab_cache_t;

/* allocate an object from cache */
addr_t slab_alloc(slab_cache_t *cache);

/* deallocate an object from cache */
void slab_free(slab_cache_t *cache, addr_t obj);

/* prepare a memory page to be used as a slab */
void slab_prepare_page(slab_cache_t *cache, addr_t page);

#endif

