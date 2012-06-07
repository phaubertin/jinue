#include <slab.h>
#include <stddef.h>

void slab_cache_create(
	slab_cache_t *cache,
	size_t       size,
	size_t       alignment,
	vm_alloc_t  *vm_allocator,
	slab_ctor_t	 ctor,
	slab_ctor_t	 dtor ) {

}

void slab_cache_destroy(slab_cache_t *cache) {
}

void *slab_cache_alloc(slab_cache_t *cache) {
	return NULL;
}

void *slab_cache_alloc_low_latency(slab_cache_t *cache) {
	return NULL;
}

void slab_cache_free(slab_cache_t *cache, void *buffer) {
}

void slab_cache_grow(slab_cache_t *cache) {
}

void slab_cache_grow_low_latency(slab_cache_t *cache) {
}

void slab_cache_reap(slab_cache_t *cache) {
}
