#ifndef _JINUE_KERNEL_SLAB_H_
#define _JINUE_KERNEL_SLAB_H_

#include <stddef.h>
#include <vm_alloc.h>

typedef void (*slab_ctor_t)(void *, size_t);


struct slab_t {};

typedef struct slab_t slab_t;

struct slab_cache_t {
	char   		*name;
	size_t       size;
	size_t       alignment;
	vm_alloc_t  *vm_allocator;
	slab_ctor_t	 ctor;
	slab_ctor_t	 dtor;
};

typedef struct slab_cache_t slab_cache_t;


void slab_cache_create(
	slab_cache_t *cache,
	size_t       size,
	size_t       alignment,
	vm_alloc_t  *vm_allocator,
	slab_ctor_t	 ctor,
	slab_ctor_t	 dtor );

void slab_cache_destroy(slab_cache_t *cache);

void *slab_cache_alloc(slab_cache_t *cache);

void *slab_cache_alloc_low_latency(slab_cache_t *cache);

void slab_cache_free(slab_cache_t *cache, void *buffer);

void slab_cache_grow(slab_cache_t *cache);

void slab_cache_grow_low_latency(slab_cache_t *cache);

void slab_cache_reap(slab_cache_t *cache);

#endif
