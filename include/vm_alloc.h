#ifndef _JINUE_VM_ALLOC_H
#define _JINUE_VM_ALLOC_H

#include <kernel.h>
#include <slab.h>
	
struct vm_link_t {
	struct vm_link_t *next;
	size_t size;
	addr_t addr;
};

typedef struct vm_link_t vm_link_t;

struct vm_alloc_t {
	size_t size;
	vm_link_t *head;
	struct slab_cache_t *cache;	
};

typedef struct vm_alloc_t vm_alloc_t;


addr_t vm_valloc(vm_alloc_t *pool);

void vm_vfree(vm_alloc_t *pool, addr_t addr);

void vm_vfree_block(vm_alloc_t *pool, addr_t addr, size_t size);

addr_t vm_alloc(vm_alloc_t *pool, unsigned long flags);

void vm_free(vm_alloc_t *pool, addr_t addr);

#endif

