#include <stddef.h>
#include <alloc.h>
#include <vm.h>
#include <vm_alloc.h> /* includes kernel.h */

addr_t vm_valloc(vm_alloc_t *pool) {
	addr_t addr;
	vm_link_t *head;
	
	head = pool->head;
	
	if(head == (addr_t)NULL) {
		return (addr_t)NULL;
	}
	
	addr = head->addr;
	(head->size) -= PAGE_SIZE;;
	
	if(head->size == 0) {
		pool->head = head->next;
		slab_free(pool->cache, head);				
		return addr;
	}
	
	(head->addr) += PAGE_SIZE;
	return addr;	
}

void vm_vfree(vm_alloc_t *pool, addr_t addr) {
	vm_vfree_block(pool, addr, PAGE_SIZE);
}

void vm_vfree_block(vm_alloc_t *pool, addr_t addr, size_t size) {
	addr_t phys_page;
	vm_link_t *link;
	
	/* The virtual space allocator needs a slab cache from which to allocate
	   data structures for its free list. Also, each slab cache needs a
	   virtual space allocator to allocate slabs when needed.
	   
	   There can be a mutual dependency between the vitual space allocator
	   and the slab cache. This is not a problem in general, but a special
	   bootstrapping procedure is needed for initialization of the virtual
	   space allocator in that case. The virtual space allocator will actually
	   "donate" a virtual page (backed by physical ram) to the cache for use as
	   a slab.
	   
	   This case is handled here
	 */
	if(pool->head == NULL) {
		if(pool->cache->vm_allocator == pool) {
			if(pool->cache->empty == NULL && pool->cache->partial == NULL) {
				phys_page = alloc(PAGE_SIZE);
				
				/*TODO: map page */
				
				size -= PAGE_SIZE;
				
				if(size == 0) {
					return;
				}
				
				addr += PAGE_SIZE;				
			}			
		}
	}
	
	link = (vm_link_t *)slab_alloc(pool->cache);
	link->size = size;
	link->addr = addr;
	
	/* TODO: make this an atomic operation */
	link->next = pool->head;
	pool->head = link;
}

addr_t vm_alloc(vm_alloc_t *pool, unsigned long flags) {
	addr_t paddr, vaddr;
	
	vaddr = vm_valloc(pool);
	paddr = alloc(PAGE_SIZE);
	vm_map(vaddr, paddr, flags);
	
	return vaddr;
}

void vm_free(vm_alloc_t *pool, addr_t addr) {
	vm_unmap(addr);
	vm_vfree(pool, addr);
}

