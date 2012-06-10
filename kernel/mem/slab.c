#include <jinue/types.h>
#include <hal/kernel.h>
#include <slab.h>
#include <vm_alloc.h>


void slab_cache_create(
	slab_cache_t *cache,
	size_t        size,
	size_t        alignment,
	slab_ctor_t	  ctor,
	slab_ctor_t	  dtor ) {
		
	cache->size = size;
	cache->ctor = ctor;
	cache->dtor = dtor;
		
	slab_cache_set_allocators(cache, NULL, NULL);
	
	if(alignment > size) {
		cache->alignment = alignment;
	}
	else {
		cache->alignment = size;
	}
}

void slab_cache_destroy(slab_cache_t *cache) {}

void *slab_cache_alloc(slab_cache_t *cache) {
	return NULL;
}

void *slab_cache_alloc_low_latency(slab_cache_t *cache) {
	return NULL;
}

void slab_cache_free(slab_cache_t *cache, void *buffer) {
}

void slab_cache_set_allocators(slab_cache_t *cache, pfcache_t *pfcache, vm_alloc_t *vma) {
	if(pfcache == NULL) {
		cache->pfcache = global_pfcache;
	}
	else {
		cache->pfcache = pfcache;
	}
	
	if(vma == NULL) {
		cache->vma = global_page_allocator;
	}
	else {
		cache->vma = vma;
	}
}

void slab_cache_grow(slab_cache_t *cache) {
}

void slab_cache_grow_low_latency(slab_cache_t *cache) {
}

void slab_cache_reap(slab_cache_t *cache) {
}
