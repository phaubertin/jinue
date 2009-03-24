#include <assert.h>
#include <alloc.h>
#include <kernel.h>
#include <slab.h>
#include <vm.h>

void slab_create(slab_cache_t *cache, unsigned long flags) {
}

void slab_destroy(slab_cache_t *cache) {
}

addr_t slab_alloc(slab_cache_t *cache) {
	slab_header_t *slab;
	addr_t addr;
	
	/* use a partial slab if one is available... */
	slab = cache->partial;
	if(slab != NULL) {
		addr = slab->free_list;
		slab->free_list = *(addr_t *)addr;
		
		/* maybe the slab is now full */
		if(--slab->available == 0) {
			slab_remove(&cache->partial, slab);
			slab_add(&cache->full, slab);
		}
		
		return addr;
	}
	
	/* ... otherwise, use an empty slab ... */
	slab = cache->empty;
	if(slab != NULL) {
		/* the slab is no longer empty */
		slab_remove(&cache->empty, slab);
		slab_add(&cache->partial, slab);
		
		addr = slab->free_list;
		slab->free_list = *(addr_t *)addr;
		
		/* maybe the slab is now full */
		if(--slab->available == 0) {
			slab_remove(&cache->partial, slab);
			slab_add(&cache->full, slab);
		}
		
		return addr;
	}
	
	/* ... and, as last resort, allocate a slab */
	/** TODO: handle the NULL pointer */
	slab = (slab_header_t *)vm_alloc(cache->vm_allocator, cache->vm_flags);
	slab_prepare(cache, (addr_t)slab);
	
	/* this slab is not empty since we are allocating an object from it */
	slab_add(&cache->partial, slab);
	
	addr = slab->free_list;
	slab->free_list = *(addr_t *)addr;
	
	/* maybe the slab is now full */
	if(--slab->available == 0) {
		slab_remove(&cache->partial, slab);
		slab_add(&cache->full, slab);
	}
	
	return addr;
}

void slab_free(slab_cache_t *cache, addr_t obj) {
}
/**
	Prepare a memory page for use as a slab. Initialize fields of the slab
	header and create the free list.
	@param cache slab cache to which the slab is to be added
	@param page memory page from which to create a slab
*/
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
	
	/** ASSERTION: we assume a physical memory page is mapped at "page" */
	assert( (*PDE_OF(page) & ~PAGE_MASK) != NULL &&  (*PDE_OF(page) & VM_FLAG_PRESENT) != 0 );
	assert( (*PTE_OF(page) & ~PAGE_MASK) != NULL &&  (*PTE_OF(page) & VM_FLAG_PRESENT) != 0 );
	
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
	Add a slab to a linked list of slabs.
	@param head of list (typically &C->empty, &C->partial or &C->full of some cache C)
	@param slab to add to list
*/
void slab_add(slab_header_t **head, slab_header_t *slab) {
	slab->next = *head;
	slab->prev = NULL;
	
	(*head)->prev = slab;	
	*head = slab;
}

/**
	Remove a slab from a linked list of slab.
	@param head of list (typically &C->empty, &C->partial or &C->full of some cache C)
	@param slab to remove from list
*/
void slab_remove(slab_header_t **head, slab_header_t *slab) {
	if(slab->next != NULL) {
		slab->next->prev = slab->prev;
	}
	
	if(slab->prev != NULL) {
		slab->prev->next = slab->next;
	}
	else {
		*head = slab->next;
	}	
}

