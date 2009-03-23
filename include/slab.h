#ifndef _JINUE_KERNEL_SLAB_H
#define _JINUE_KERNEL_SLAB_H

#include <vm_alloc.h> /* includes kernel.h */

/** header of a slab */
struct slab_header_t {
	/** number of available objects in free list */
	count_t    available;
	
	/** head of the free list */
	addr_t     free_list;
	
	/** pointer to next slab in linked list */
	struct slab_header_t *next;
	
	/** pointer to previous slab in linked list */
	struct slab_header_t *prev;	
};

typedef struct slab_header_t slab_header_t;

/** data structure describing a cache */
struct slab_cache_t {
	/** size of objects to allocate */
	size_t  obj_size;
	
	/** number of objects per slab */
	count_t per_slab;
	
	/** head of list of empty slabs */
	slab_header_t *empty;
	
	/** head of list of partial slabs */
	slab_header_t *partial;
	
	/** head of list of full slabs */
	slab_header_t *full;
	
	/** virtual address space allocator for new slabs */
	struct vm_alloc_t *vm_allocator;
};

typedef struct slab_cache_t slab_cache_t;

/* allocate an object from cache */
addr_t slab_alloc(slab_cache_t *cache);

/* deallocate an object from cache */
void slab_free(slab_cache_t *cache, addr_t obj);

/* prepare a memory page to be used as a slab */
void slab_prepare(slab_cache_t *cache, addr_t page);

/* insert a slab in a list of slabs */
void slab_insert(slab_header_t **head, slab_header_t *slab);

#endif

