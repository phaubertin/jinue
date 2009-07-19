#include <alloc.h>
#include <assert.h>
#include <slab.h>
#include <stddef.h>
#include <vm.h>
#include <vm_alloc.h> /* includes kernel.h */

vm_alloc_t   global_pool;

slab_cache_t global_pool_cache;


void vm_create_pool(vm_alloc_t *pool, slab_cache_t *cache) {
	pool->size = 0;
	pool->head = NULL;
	pool->cache = cache;
}

/**
	Allocate a page of virtual memory (not backed by physical memory). This
	page may then be used for temporary mappings, for example. Page is
	allocated from a specific virtual memory region managed by a vm_alloc_t
	data structure.
	@param pool data structure managing the virtual memory region from which to
	            allocate
	@return address of allocated page 
*/
addr_t vm_valloc(vm_alloc_t *pool) {
	addr_t addr;
	vm_link_t *head;
	size_t size;
	
	head = pool->head;
	
	/* no page available */
	if(head == (addr_t)NULL) {
		return (addr_t)NULL;
	}
	
	addr = head->addr;
	size = head->size - PAGE_SIZE;
	
	/** ASSERTION: block size should be an integer number of pages */
	assert( PAGE_OFFSET_OF(size) == 0 );
	
	/* if block is made of only one page, we remove it from the free list */
	if(size == 0) {
		pool->head = head->next;
		slab_free(pool->cache, head);				
	}
	else {
		head->size = size;
		head->addr += PAGE_SIZE;
	}
	
	/** ASSERTION: returned address should be aligned with a page boundary */
	assert( PAGE_OFFSET_OF(addr) == 0 );
	
	return addr;	
}

/**
	Return a single page of virtual memory to a pool of available pages. Should
	not be used to free pages to which physical memory is still mapped
	(no physical memory is freed by this function). Use this function to return
	pages obtained by a call to vm_valloc() (and not vm_alloc()).
	@param pool data structure managing the relevant virtual memory region
	@param addr address of virtual page which must be freed
*/
void vm_vfree(vm_alloc_t *pool, addr_t addr) {
	vm_vfree_block(pool, addr, PAGE_SIZE);
}

/**
	Return a block of contiguous virtual memory pages to a pool of available
	pages. Should not be used to free pages to which physical memory is still 
	mapped (no physical memory is freed by this function).
	@param pool data structure managing the relevant virtual memory region
	@param addr starting address of virtual memory block
	@param size size of block
*/
void vm_vfree_block(vm_alloc_t *pool, addr_t addr, size_t size) {
	addr_t phys_page;
	vm_link_t *link;
	
	/** ASSERTION: we assume starting address is aligned on a page boundary */
	assert( PAGE_OFFSET_OF(addr) == 0 );
	
	/** ASSERTION: we assume size of block is an integer number of pages */
	assert( PAGE_OFFSET_OF(size) == 0 );
	
	/** ASSERTION: address of block should not be the null pointer */
	assert( addr != (addr_t)NULL );
	
	/*  The virtual address space allocator needs a slab cache from which to
		allocate data structures for its free list. Also, each slab cache needs
		a virtual address space allocator to allocate slabs when needed.
	   
		There can be a mutual dependency between the virtual address space
		allocator and the slab cache. This is not a problem in general, but a
		special	bootstrapping procedure is needed for initialization of the
		virtual address space allocator in that case. The virtual address space
		allocator will actually "donate" a virtual page (backed by physical ram)
		to the cache for use as a slab.
	   
		This case is handled here
	 */
	if(pool->head == NULL) {
		if(pool->cache->vm_allocator == pool) {
			if(pool->cache->empty == NULL && pool->cache->partial == NULL) {
				/* allocate a physical page for slab */
				phys_page = alloc(PAGE_SIZE);
				
				/* map page */
				vm_map(addr, phys_page, VM_FLAG_KERNEL);
				
				/* prepare the slab and add it to cache empty list */
				slab_prepare(pool->cache, addr);
				slab_add(&pool->cache->empty, addr);
				
				size -= PAGE_SIZE;
				
				/* if the block contained only one page, we have nothing left
				   to free */
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
	
	link->next = pool->head;
	pool->head = link;
}
/**
	Allocate a physical memory page and map it in virtual memory.
	@param pool data structure managing the virtual memory region in which page will be mapped
	@param flags flags for page mapping (passed as-is to vm_map())
*/
addr_t vm_alloc(vm_alloc_t *pool, unsigned long flags) {
	addr_t paddr, vaddr;
	
	/** TODO: handle the NULL pointer */
	
	vaddr = vm_valloc(pool);
	paddr = alloc(PAGE_SIZE);
	vm_map(vaddr, paddr, flags);
	
	return vaddr;
}

/**
	Free a physical page mapped in virtual memory (which was typically obtained
	through	a call to vm_map()). The physical memory is freed and the virtual
	page is	returned to the virtual address space allocator.
	@param pool data structure managing the virtual memory region to which the page is returned
	@addr address of page to free
*/
void vm_free(vm_alloc_t *pool, addr_t addr) {
	addr_t paddr;
	
	/** ASSERTION: address of page should not be the null pointer */
	assert( addr != (addr_t)NULL );
	
	paddr = (addr_t)(*PTE_OF(addr) | ~PAGE_MASK);	
	
	vm_unmap(addr);
	vm_vfree(pool, addr);
	free(paddr);
}

