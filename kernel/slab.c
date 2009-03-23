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

void slab_prepare(slab_cache_t *cache, addr_t page) {
	unsigned int cx;
	size_t obj_size;
	count_t per_slab;
	slab_header_t *slab;
	addr_t *ptr;
	addr_t next;
	
	/** ASSERTION: we assume "page" is the starting address of a page */
	assert( PAGE_OFFSET_OF(page) == 0 );
	
	/** ASSERTION: we assume at least one object can be allocated on slab */
	assert( cache->per_slab > 0 );
	
	obj_size = cache->obj_size;
	per_slab = cache->per_slab;

	/* initialize slab header */
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
}

/**
	Insert a slab in a linked list of slabs.
	@param head of list (typically &C->empty, &C->partial or &C->full of some cache C)
	@param slab to add to list
*/
void slab_insert(slab_header_t **head, slab_header_t *slab) {
	slab->next = *head;
	slab->prev = NULL;
	
	(*head)->prev = slab;	
	*head = slab;
}

