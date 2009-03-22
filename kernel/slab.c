#include <assert.h>
#include <alloc.h>
#include <kernel.h>
#include <slab.h>
#include <vm.h>

addr_t slab_alloc(slab_cache_t *cache) {
	return NULL;
}

void slab_free(slab_cache_t *cache, addr_t obj) {
}

void slab_prepare_page(slab_cache_t *cache, addr_t page) {
	unsigned int cx;
	size_t obj_size;
	count_t per_slab;
	slab_header_t *slab;
	addr_t *ptr;
	addr_t next;
	
	/* we assume "page" is the starting address of a page */
	assert( PAGE_OFFSET_OF(page) );
	
	obj_size = cache->obj_size;
	per_slab = cache->per_slab;

	slab = (slab_header_t *)page;
	slab->available = per_slab;
	slab->free_list = page + sizeof(slab_header_t);
	
	/* create free list */
	ptr = (addr_t *)slab->free_list;
	
	for(cx = 0; cx < per_slab - 1; ++cx) {
		next = ptr + obj_size;
		*ptr = next;
		ptr = (addr_t *)next;		
	}
	
	*ptr = NULL;
	
	/* insert in list of empty slabs of cache */
	slab->prev = NULL;
	slab->next = cache->empty;
	cache->empty = slab;
}

