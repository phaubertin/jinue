#ifndef _JINUE_KERNEL_VM_ALLOC_H
#define _JINUE_KERNEL_VM_ALLOC_H

#include <kernel.h>
#include <slab.h>
	
/** links forming the linked lists of free virtual memory pages */
struct vm_link_t {
	/** next link in list */
	struct vm_link_t *next;
	
	/** size of current virtual memory block */
	size_t size;
	
	/** starting address of current block */
	addr_t addr;
};

typedef struct vm_link_t vm_link_t;

/** data structure which keep tracks of free pages in a region of virtual
	memory */
struct vm_alloc_t {
	/** total amount of memory available  */
	size_t size;
	
	/** head of the free list */
	vm_link_t *head;
	
	/** slab cache on which to allocate the links of the free list */
	struct slab_cache_t *cache;	
};

typedef struct vm_alloc_t vm_alloc_t;


addr_t vm_valloc(vm_alloc_t *pool);

void vm_vfree(vm_alloc_t *pool, addr_t addr);

void vm_vfree_block(vm_alloc_t *pool, addr_t addr, size_t size);

addr_t vm_alloc(vm_alloc_t *pool, unsigned long flags);

void vm_free(vm_alloc_t *pool, addr_t addr);

#endif

