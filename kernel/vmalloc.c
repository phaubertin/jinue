/*
 * Copyright (C) 2019 Philippe Aubertin.
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the author nor the names of other contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <hal/vm.h>
#include <assert.h>
#include <pfalloc.h>
#include <stdbool.h>
#include <stddef.h>
#include <types.h>
#include <util.h>
#include <vmalloc.h>

#define VMALLOC_STACK_ENTRIES   1024

#define VMALLOC_BLOCK_SIZE      (VMALLOC_STACK_ENTRIES * PAGE_SIZE)

#define VMALLOC_BLOCK_MASK      (VMALLOC_BLOCK_SIZE - 1)


#define VMALLOC_IS_FREE(b)      ((b)->next != NULL && (b)->stack_ptr == NULL)

#define VMALLOC_IS_PARTIAL(b)   ((b)->next != NULL && (b)->stack_ptr != NULL)

#define VMALLOC_IS_USED(b)      ((b)->next == NULL)


#define VMALLOC_EMPTY_STACK(b)  ((b)->stack_ptr >= (b)->stack_addr + VMALLOC_STACK_ENTRIES)

#define VMALLOC_FULL_STACK(b)   ((b)->stack_ptr <= (b)->stack_addr + 1)

#define VMALLOC_CANNOT_GROW(b)  ((b)->stack_next >= (b)->base_addr + VMALLOC_BLOCK_SIZE)


struct vmalloc_t {
    /** base address of memory managed by allocator */
    addr_t base_addr;

    /** start address of memory actually available to the allocator */
    addr_t start_addr;

    /** end address of memory actually available to the allocator */
    addr_t end_addr;

    /** number of memory blocks managed by this allocator */
    unsigned int block_count;

    /** array of memory block descriptors */
    struct vmalloc_block_t *block_array;

    /** number of pages allocated for block array */
    unsigned int array_pages;

    /** list of completely free blocks */
    struct vmalloc_block_t *free_list;

    /** list of partially free blocks */
    struct vmalloc_block_t *partial_list;
};

struct vmalloc_block_t {
    /** base address of memory block */
    addr_t base_addr;

    /** allocator to which this block belongs */
    struct vmalloc_t *allocator;

    /** stack pointer for stack of free pages in partially allocated blocks */
    addr_t *stack_ptr;

    /** base address of free page stack */
    addr_t *stack_addr;

    /** next page address to add to stack, used for deferred stack initialization */
    addr_t stack_next;

    /** link previous block in free list */
    struct vmalloc_block_t *prev;

    /** link next block in free list */
    struct vmalloc_block_t *next;
};

static vmalloc_t __global_page_allocator;

/** global page allocator (region 0..KLIMIT) */
vmalloc_t *const global_page_allocator = &__global_page_allocator;

static void vmalloc_free_block(vmalloc_block_t *block);

static void vmalloc_partial_block(vmalloc_block_t *block);

static void vmalloc_custom_block(vmalloc_block_t *block, addr_t start_addr, addr_t end_addr);

static void vmalloc_unlink_block(vmalloc_block_t *block);

static void vmalloc_grow_stack(vmalloc_block_t *block);

/**
  @file
  
  Virtual memory allocator
  
  Functions in this file are used to manage the virtual address space. Each
  region of the address space is represented by a vm_alloc_t structure.

  Pages are allocated one at a time. There is no way to allocate groups of
  contiguous pages in the kernel.
  
  Address space regions are split in 4MB-sized, 4MB-aligned blocks (1024 pages),
  each represented by vm_block_t structures. Each block may be either free (all
  pages available for allocation), partial (some pages available) or used (all
  pages allocated). For partial blocks, a page is used as a page stack for
  fast allocation and de-allocation. 
  
  vm_block_t structures for an address space region are placed in an array at
  the start of region. This array is used to quickly find the right vm_block_t
  structure during de-allocations. There is also a free block list (the free
  list) and a partial block list (the partial list) for each region (circular
  doubly-linked lists), which allows the allocator to quickly find a block with
  free pages during allocations.
  
  Some implementation details:
  
  * Page stacks grow downward. We pre-decrement when de-allocating (adding pages
    on top of the stack) and post-increment when allocating (removing pages from
    the stack). This means the stack pointer points to the next allocatable page.
  
  * The prev and next members of vm_block_t link the block to the partial or
    free list (if applicable), and the stack member is the stack pointer for
    partial blocks. If the next member is NULL, then the block is unlinked,
    otherwise it is linked either to the free or the partial list. When the
    block is unlinked, the prev and stack_ptr members are undefined (probably
    not NULL). When the block is linked, either the stack_ptr member is NULL,
    in which case the block is free (linked to the free list), or it is non-NULL,
    in which case it is a partial block (linked to the partial list).
*/

/**
    Allocate a page of virtual address space.
    @param allocator allocator which manages the memory region from which we wish to obtain a page
*/
addr_t vmalloc(vmalloc_t *allocator) {
    vmalloc_block_t *block;
    addr_t      page;
    
    /** ASSERTION: allocator is not null */
    assert(allocator != NULL);
    
    block = allocator->partial_list;
    
    if(block == NULL) {
        block = allocator->free_list;
        
        if(block == NULL) {
            return NULL;
        }
        
        vmalloc_partial_block(block);
    }
    
    /** ASSERTION: since block is expected to be partial, its stack pointer should not be null */
    assert(block->stack_ptr != NULL);
    
    /* if the page stack is empty, perform deferred page stack initialization */
    if( VMALLOC_EMPTY_STACK(block) ) {
        vmalloc_grow_stack(block);
    }
    
    /** ASSERTION: at this point, the page stack should not be empty (stack underflow check) */
    assert( ! VMALLOC_EMPTY_STACK(block) );
    
    page = *(block->stack_ptr++);
    
    /* if we just emptied the stack, mark the block as used */
    if( VMALLOC_EMPTY_STACK(block) ) {
        vmalloc_unlink_block(block);
    }

    return page;
}

/**
    Free a page of virtual address space.
    @param allocator allocator which manages the memory region to which the page is freed
*/
void vmfree(vmalloc_t *allocator, addr_t page) {
    vmalloc_block_t   *block;
    unsigned int  idx;
    
    /** ASSERTION: allocator is not null */
    assert(allocator != NULL);
    
    /** ASSERTION: ensure we are freeing to the proper allocator/region */
    assert(page >= allocator->start_addr && page < allocator->end_addr);
    
    /** ASSERTION: ensure address is page aligned */
    assert(page_offset_of(page) == 0);
    
    /* find the block to which the free page belong */
    idx = ( (unsigned int)page - (unsigned int)allocator->base_addr ) / VMALLOC_BLOCK_SIZE;
    block = &allocator->block_array[idx];
    
    /* if the block was a used block, make it a partial block */
    if( VMALLOC_IS_USED(block) ) {
        if(block->stack_addr == NULL) {
            block->stack_addr = (addr_t *)page;
            return;
        }
        
        vmalloc_partial_block(block);
    }
    
    /** ASSERTION: block should now be partial */
    assert( VMALLOC_IS_PARTIAL(block) );
    
    /** ASSERTION: stack overflow check */
    assert( ! VMALLOC_FULL_STACK(block) );
    
    *(--block->stack_ptr) = page;
    
    /* check if we just freed the whole block */
    if( VMALLOC_FULL_STACK(block) ) {
        vmalloc_free_block(block);
    }
}

void vmalloc_init(vmalloc_t *allocator, addr_t start_addr, addr_t end_addr) {
    vmalloc_init_allocator( allocator, start_addr, end_addr);
    vmalloc_add_region(     allocator, start_addr, end_addr);
}

/**
    Basic initialization of virtual memory allocator
    @param allocator  vm_alloc_t structure for a virtual memory allocator
    @param start_addr start address of the region managed by the allocator
    @param size       size of the region managed by the allocator
*/
void vmalloc_init_allocator(vmalloc_t *allocator, addr_t start_addr, addr_t end_addr) {
    addr_t           base_addr;         /* block-aligned start address */
    addr_t           aligned_end;       /* block-aligned end address */
    addr_t           adjusted_start;    /* actual start of available memory, block array skipped */
    
    vmalloc_block_t      *block_array;       /* start of array */
    unsigned int     block_count;       /* array size, in blocks (entries) */
    size_t           array_size;        /* array size, in bytes */
    unsigned int     array_page_count;  /* array size, in pages */
    
    addr_t           addr;              /* some virtual address */
    kern_paddr_t     paddr;             /* some page frame address */
    unsigned int     idx;               /* an array index */
        
    
    /** ASSERTION: allocator structure pointer must not be null */
    assert(allocator != NULL);
    
    /** ASSERTION: start and end addresses must be multiples of page size (page-aligned memory region) */
    assert( page_offset_of(start_addr) == 0 && page_offset_of(end_addr) == 0 );
    
    
    /* align base and end addresses to block size */
    base_addr   = (addr_t)ALIGN_START(start_addr, VMALLOC_BLOCK_SIZE);
    aligned_end = (addr_t)ALIGN_END(end_addr, VMALLOC_BLOCK_SIZE);

    /* calculate number of memory blocks managed by this allocator */
    block_count = ( (char *)aligned_end - (char *)base_addr ) / VMALLOC_BLOCK_SIZE;
    
    /* calculate the number of pages required to store the memory block
     * descriptor array */
    array_size = block_count * sizeof(vmalloc_block_t);
    array_page_count = array_size / PAGE_SIZE;
    if(array_size % PAGE_SIZE != 0) {
        ++array_page_count;
    }
    
    /* address of the block array */
    block_array = (vmalloc_block_t *)start_addr;
    
    /* adjust base address to skip block descriptor array */
    adjusted_start = start_addr + array_page_count * PAGE_SIZE;
    
    /* initialize allocator struct */
    allocator->start_addr   = adjusted_start;
    allocator->end_addr     = end_addr;
    allocator->base_addr    = base_addr;
    allocator->block_count  = block_count;
    allocator->block_array  = block_array;
    allocator->array_pages  = array_page_count;
    allocator->free_list    = NULL;
    allocator->partial_list = NULL;
    
    /* allocate block descriptor array pages */
    addr = (addr_t)block_array;
    for(idx = 0; idx < array_page_count; ++idx) {
        /* allocate and map page */
        paddr = pfalloc();
        vm_map_kernel(addr, paddr, VM_FLAG_READ_WRITE);
        
        /* calculate address of next page */
        addr += PAGE_SIZE;
    }
    
    /** ASSERTION: once all the array pages are allocated, we should have reached the allocatable pages region */
    assert(addr == adjusted_start);
    
    /* basic initialization of array (all blocks unlinked/used) */
    addr = base_addr;
    for(idx = 0; idx < block_count; ++idx) {
        block_array[idx].base_addr  = addr;
        block_array[idx].allocator  = allocator;
        
        /* mark block as unlinked for now */
        block_array[idx].next       = NULL;
        
        /* a null stack base indicates the block is uninitialized */
        block_array[idx].stack_addr = NULL;
        
        /* calculate address of next block */
        addr += VMALLOC_BLOCK_SIZE;
    }
}

/** 
    Add a contiguous region of available virtual memory to the allocator.
    @param allocator  vm_alloc_t structure for a virtual memory allocator
    @param start_addr start address of the region
    @param end_addr   end address of the region (first unavailable page)
*/
void vmalloc_add_region(vmalloc_t *allocator, addr_t start_addr, addr_t end_addr) {
    addr_t       start_addr_adjusted;
    unsigned int start;
    unsigned int end;
    unsigned int end_full;
    unsigned int idx;
    addr_t       limit;

    /* skip the block array */
    if(start_addr >= allocator->start_addr) {
        start_addr_adjusted = start_addr;
    }
    else {
        start_addr_adjusted = allocator->start_addr;
    } 
    
    /* start and end block indices */
    start = ((unsigned int)start_addr_adjusted - (unsigned int)allocator->base_addr) / VMALLOC_BLOCK_SIZE;    
    end   = ((unsigned int)end_addr            - (unsigned int)allocator->base_addr) / VMALLOC_BLOCK_SIZE;
    
    /* check and remember whether last block is partial (last_full < end) or
     * completely free (last_full == end) */
    if( OFFSET_OF(end_addr, VMALLOC_BLOCK_SIZE) == 0) {
        end_full = end;
    }
    else {
        end_full = end + 1;
    }
    
    /* array initialization -- first block (if partial) */
    idx = start;
    
    if( OFFSET_OF(start_addr_adjusted, VMALLOC_BLOCK_SIZE) != 0 ) {
        limit = ALIGN_END(start_addr_adjusted, VMALLOC_BLOCK_SIZE);
        
        if(end_addr < limit) {
            limit = end_addr;
        }
        
        vmalloc_custom_block(&allocator->block_array[idx], start_addr_adjusted, limit);
        
        ++idx;
    }
    
    /* array initialization -- free blocks */
    for(; idx < end; ++idx) {
        vmalloc_free_block(&allocator->block_array[idx]);
    }
    
    /* array initialization -- last block (if partial) */
    if(idx < end_full) {
        vmalloc_custom_block(&allocator->block_array[idx], allocator->block_array[idx].base_addr, end_addr);
    }
}


/**
    Insert block in the free list.

    This is typically done when the block was a partial one, and the last page
    has just been returned to it.
    @param block block to insert in the free list
*/
static void vmalloc_free_block(vmalloc_block_t *block) {
    vmalloc_block_t      *prev;
    vmalloc_block_t      *next;
    
    /** ASSERTION: block is not null */
    assert(block != NULL);
    
    /* unlink from partial list if necessary */
    vmalloc_unlink_block(block);
    
    /** ASSERTION: block->allocator should not be NULL */
    assert(block->allocator != NULL);
    
    /* link block to the free list */
    if(block->allocator->free_list == NULL) {
        /* special case: free list is empty */
        block->allocator->free_list = block;
        
        block->next = block;
        block->prev = block;
    }
    else {
        /* block will be at the end of the free list */
        next = block->allocator->free_list;
        prev = next->prev;
        
        /* re-link block */
        block->prev = prev;
        block->next = next;
        
        prev->next = block;
        next->prev = block;        
    }
    
    /* set the stack pointer to null to indicate this is a free block */
    block->stack_ptr = NULL;
}

/**
    Insert block in the partial blocks list.

    This is typically done when the block is a free one from which we intend to
    allocate pages, or when the block is used (unlinked) and we intend to return
    pages to it. The stack is initialized empty, but the deferred stack
    initialization mechanism is enabled if the block is free on function entry.
    @param block block to insert in the partial list
*/
static void vmalloc_partial_block(vmalloc_block_t *block) {
    vmalloc_block_t      *prev;
    vmalloc_block_t      *next;
    addr_t          *stack_addr;
    kern_paddr_t     paddr;
    bool             was_free;
    
    
    /** ASSERTION: block is not null */
    assert(block != NULL);
    
    /* To keep in mind...
     * 
     * When the allocator is initialized, some blocks may be created partial
     * (typical for the first and the last block of the region). If there is a
     * hole at the start of the block, the page stack will be at the first
     * available page, not at the start of the block. Since these blocks have
     * holes, they will never be in the free state.
     * 
     * So, when a block is free on function entry, we ensure the stack is placed
     * at the start of the block so that all the remaining pages can be
     * allocated sequentially (see deferred stack intialization below). However,
     * if the block is in the used state on function entry, we leave the stack
     * at its previous location since the first page of the block might not be
     * available. */
    
    if(block->next == NULL) {
        /** ASSERTION: block stack address is not null  */
        assert(block->stack_addr != NULL);
        
        /* block was used on function entry */
        was_free = false;
    }
    else {
        if(block->stack_ptr != NULL) {
            /* block is already partial, leave it untouched */
            return;
        }
        
        /* block was free on function entry */
        was_free = true;
        
        /* unlink from free list */
        vmalloc_unlink_block(block);
        
        /* use first page of block for the stack */
        block->stack_addr = (addr_t *)block->base_addr;
    }
    
    /* allocate the page stack */
    stack_addr  = block->stack_addr;
    paddr       = pfalloc();
    vm_map_kernel((addr_t)stack_addr, paddr, VM_FLAG_READ_WRITE);
    
    /** ASSERTION: block->allocator should not be NULL*/
    assert(block->allocator != NULL);
    
    /* link block to the partial list */
    if(block->allocator->partial_list == NULL) {
        /* special case: partial list is empty */
        block->allocator->partial_list = block;
        
        block->next = block;
        block->prev = block;
    }
    else {
        /* block will be at to the end of the partial block list */
        next = block->allocator->partial_list;
        prev = next->prev;
        
        /* re-link block */
        block->prev = prev;
        block->next = next;
        
        prev->next = block;
        next->prev = block;        
    }
    
    /* Ok, here's the deal (deferred stack intialization)...
     * 
     * We do not want to initialize the page stack right now because this is
     * a time consuming operation, and we might be in time-critical code
     * (interrupt handling code for example). Instead, the stack initialization
     * is deferred until the next page allocations. The first non-time critical
     * allocation which encounters an empty stack will initialize the whole
     * stack. In the meantime, time critical ones will just allocate pages
     * sequentially from the start of the block.
     * 
     * The stack_next pointer in the vm_block_t structure points to the next
     * page available for sequential allocation. The memory block is actually
     * used up (no more pages available) when the page stack is empty AND the
     * stack_next pointer has reached the end of the block. */
    
    /* initialize the stack as empty */
    block->stack_ptr = (addr_t *)( (char *)stack_addr + PAGE_SIZE );
    
    if(was_free) {
        /* free block: we skip the first page as it was allocated for the
         * stack itself */
        block->stack_next = block->base_addr + PAGE_SIZE;
    }
    else {
        /* used block: sequential allocation no longer possible */
        block->stack_next = block->base_addr + VMALLOC_BLOCK_SIZE;
    }
}

static void vmalloc_custom_block(vmalloc_block_t *block, addr_t start_addr, addr_t end_addr) {
#ifndef NDEBUG
    addr_t      limit;
#endif
    addr_t      page;
    addr_t      adjusted_start;

    /** ASSERTION: block is not null */
    assert(block != NULL);
    
    /** ASSERTION: start and end addresses must be page aligned */
    assert(page_offset_of(start_addr) == 0 && page_offset_of(end_addr) == 0);

#ifndef NDEBUG
    limit = block->base_addr + VMALLOC_BLOCK_SIZE;
#endif

    /** ASSERTION: start and end addr are inside block, address range is non-empty */
    assert(start_addr >= block->base_addr && end_addr <= limit && start_addr < end_addr );
    
    /** ASSERTION: block is not free */
    assert( ! VMALLOC_IS_FREE(block) );
    
    adjusted_start = start_addr;
    
    if( VMALLOC_IS_USED(block) ) {
        /* if no stack address is specified at this point, use the first page
         * of the address range for this purpose */
        if( block->stack_addr == NULL ) {
            block->stack_addr = (addr_t *)start_addr;
            adjusted_start    = start_addr + PAGE_SIZE;
            
            /* if the address range contained only a single page, there is
             * nothing left to do here */
            if(adjusted_start >= end_addr) {
                return;
            }
        }
        
        vmalloc_partial_block(block);
    }
    
    /** ASSERTION: block is partial at this point */
    assert( VMALLOC_IS_PARTIAL(block) );
    
    /* initialize stack */
    page = adjusted_start;
    while(page < end_addr) {
        /** ASSERTION: page stack overflow check */
        assert( ! VMALLOC_FULL_STACK(block) );
        
        *(--block->stack_ptr) = page;
        page += PAGE_SIZE;
    }
}

/**
    Unlink memory block from free or partial block list. It is not an error if
    block is not linked to either list. On exit of this funtion, the block is
    in the used state.
    @param block block to unlink from list
*/
static void vmalloc_unlink_block(vmalloc_block_t *block) {
    vmalloc_t      *allocator;
    kern_paddr_t     paddr;
    
    /** ASSERTION: block is not null */
    assert(block != NULL);
    
    /** ASSERTION: block is either properly linked (no null pointers) or not at
     *             all (next is null) */
    assert(block->prev != NULL || block->next == NULL);
    
    /** ASSERTION: block->allocator should not be NULL */
    assert(block->allocator != NULL);
    
    /* get allocator for block (required for next assert as well as subsequent code) */
    allocator = block->allocator;
    
    /** ASSERTION: block should not be the head of both free and partial lists */
    assert(allocator->free_list != block || allocator->partial_list != block);
    
    /* if block is already unlinked, we have nothing to do here */
    if(block->next == NULL) {
        return;
    }
    
    /* if block has a stack, discard it */
    if(block->stack_ptr != NULL) {
        paddr = vm_lookup_kernel_paddr((addr_t)block->stack_addr);
        pffree(paddr);
    }

    /* special case: block is alone in its list */
    if(block->next == block) {
        /** ASSERTION: if block is alone in its list, the previous node pointer
         *             should point to self */
        assert(block->prev == block);
        
        /** ASSERTION: if block is alone in its list,  we expect it to be the
         *             head of either the free or the partial list */
        assert(allocator->free_list == block || allocator->partial_list == block);
        
        if(allocator->free_list == block) {
            allocator->free_list = NULL;
        }
        
        if(allocator->partial_list == block) {
            allocator->partial_list = NULL;
        }
    }
    else {
        if(allocator->free_list == block) {
            allocator->free_list = block->next;
        }
            
        if(allocator->partial_list == block) {
            allocator->partial_list = block->next;
        }
        
        /* unlink block */
        block->next->prev = block->prev;
        block->prev->next = block->next;
    }
    
    /* set next pointer to null to indicate block is unlinked */
    block->next = NULL;
}

/**
    Initialize the stack of a partial block with all remaining pages that have
    not yet been allocated.
    @param block block which will have its stack initialized
*/
static void vmalloc_grow_stack(vmalloc_block_t *block) {
    addr_t  limit;
    addr_t  page;
    addr_t *stack_ptr;
    
    /** ASSERTION: block is not null */
    assert(block != NULL);
    
    /** ASSERTION: block is linked (it should be in the partial list) */
    assert(block->next != NULL && block->prev != NULL);
    
    /** ASSERTION: block actually has a stack */
    assert(block->stack_ptr != NULL);
    
    stack_ptr = block->stack_ptr;
    page      = block->stack_next;
    limit     = block->base_addr + VMALLOC_BLOCK_SIZE;
        
    while(page < limit) {
        /** ASSERTION: stack underflow check */
        assert( ! VMALLOC_FULL_STACK(block) );
        
        *(--stack_ptr) = page;
        
        page += PAGE_SIZE;
    }
    
    block->stack_ptr  = stack_ptr;
    block->stack_next = limit;
}
