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
#include <boot.h>
#include <page_alloc.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <types.h>
#include <util.h>
#include <vmalloc.h>


/**
 * @file
 *
 * Virtual address space allocator
 *
 * Functions in this file are used to manage the allocation of pages in a
 * virtual address space region.
 *
 * The kernel address space has only one region where allocations are managed by
 * this allocator, the so-called allocations region. A different allocator is
 * used during initialization - see boot.c. That other allocator manages a
 * different, smaller region of the address space, the so-called image region.
 *
 * While this allocation manages a single region of the address space, it is
 * structured in such a way that it can easily be used to manage multiple
 * address space regions independently, with each region being represented by a
 * vmalloc_t structure.
 *
 * This allocator allocates pages one at a time. There is no way to allocate
 * multiple contiguous pages after kernel initialization. (However, this is
 * something the initialization-time allocator can do.
 *
 * The allocator manages its address space region in identically-sized blocks of
 * a few megabytes aligned on their size. Each block is represented by a
 * vmalloc_block_t structure and has a stack to record available pages. The size
 * of this stack is exactly one page, which is what determines the block size.
 *
 * vm_block_t structures for an allocator are in an array that ensures the right
 * vm_block_t structure can be found quickly during de-allocation. Non-depleted
 * blocks are linked to a free list (a circular, doubly-linked list) that allows
 * the allocator to quickly find a block with free pages during allocations.
 *
 * Warning: the prev member (back pointer) of vmalloc_block_t is only meaningful
 *         while the block is linked to the free list. If there is a need to
 *         check whether the block is linked or not and vmalloc_stack_is_empty()
 *         isn't appropriate, check if the next member (not prev) is NULL.
 * */

#define VMALLOC_STACK_ENTRIES   (PAGE_SIZE / sizeof(addr_t))

#define VMALLOC_BLOCK_SIZE      (VMALLOC_STACK_ENTRIES * PAGE_SIZE)


typedef struct {
    /** start address of the first block managed by the allocator */
    addr_t base_addr;

    /** start address of the address space region managed by the allocator */
    addr_t start_addr;

    /** end address of the address space region managed by the allocator */
    addr_t end_addr;

    /** number of memory blocks managed by this allocator */
    unsigned int block_count;

    /** array of memory block descriptors */
    struct vmalloc_block_t *block_array;

    /** list of completely or partially free blocks */
    struct vmalloc_block_t *free_list;
} vmalloc_t;

struct vmalloc_block_t {
    /** stack pointer for stack of free pages in partially allocated blocks */
    addr_t *stack_ptr;

    /** base address of free page stack */
    addr_t *stack_base;

    /** link previous block in free list */
    struct vmalloc_block_t *prev;

    /** link next block in free list */
    struct vmalloc_block_t *next;
};

typedef struct vmalloc_block_t vmalloc_block_t;


/** Allocator for the allocations region of the kernel address space. */
static vmalloc_t kernel_vmallocator;


static addr_t vmalloc_from(vmalloc_t *allocator);

static void vmfree_to(vmalloc_t *allocator, addr_t page);

static void init_allocator(
        vmalloc_t       *allocator,
        addr_t           start_addr,
        addr_t           end_addr,
        addr_t           preinit_limit,
        boot_alloc_t    *boot_alloc);

static bool addr_is_in_allocator_range(const vmalloc_t *allocator, addr_t page);


/**
 * Allocate a page of virtual address space.
 *
 * @return address of allocated page or NULL if allocation failed
 *
 * */
addr_t vmalloc(void) {
    return vmalloc_from(&kernel_vmallocator);
}

/**
 * Free a page of virtual address space.
 *
 * @param page the address of the page to free
 *
 * */
void vmfree(addr_t page) {
    vmfree_to(&kernel_vmallocator, page);
}

/**
 * Basic initialization of the virtual memory allocator.
 *
 * This function initializes the allocator structure, and then initializes the
 * first few blocks up to the limit set by the preinit_limit argument (more
 * precisely, up to and including the block that contains preinit_limit - 1).
 * TODO mention how to initialize the rest once this is implemented.
 *
 * @param start_addr the start address of the region managed by the allocator
 * @param end_addr the end address of the region managed by the allocator
 * @param preinit_limit the limit address for preinitialized blocks
 * @param boot_alloc the initialization-time page allocator structure
 *
 * */
void vmalloc_init(
        addr_t           start_addr,
        addr_t           end_addr,
        addr_t           preinit_limit,
        boot_alloc_t    *boot_alloc) {

    init_allocator(
            &kernel_vmallocator,
            start_addr,
            end_addr,
            preinit_limit,
            boot_alloc);
}

/**
 * Check whether the specified page is in the region managed by the allocator.
 *
 * @param page the address of the page
 * @return true if it is in the region, false otherwise
 *
 * */
bool vmalloc_is_in_range(addr_t page) {
    return addr_is_in_allocator_range(&kernel_vmallocator, page);
}


static void vmalloc_link_block(vmalloc_t *allocator, vmalloc_block_t *block);

static void vmalloc_unlink_block(vmalloc_t *allocator, vmalloc_block_t *block);

static void vmalloc_initialize_block(vmalloc_t *allocator, unsigned int block_index, void *stack_page);

/**
 * Check if a block's page stack is empty.
 *
 * When a block's page stack is empty, the block is depleted (i.e. full) and
 * more pages cannot be allocated from it.
 *
 * @param block the block
 * @return true if the stack is empty, false if pages can still be allocated
 *
 * */
static inline bool vmalloc_stack_is_empty(const vmalloc_block_t *block) {
    return block->stack_ptr >= block->stack_base + VMALLOC_STACK_ENTRIES;
}

/**
 * Push a page on a block's page stack.
 *
 * Used when freeing a page back to the allocator or when initializing a block.
 *
 * The page stacks grows downward: we pre-decrement when pushing pages on the
 * stack and post-increment when popping a page from the stack. The stack
 * pointer points to the next allocatable page.
 *
 * @param block the block whose page stack is to be manipulated
 * @param page the page to push on the stack
 *
 * */
static inline void vmalloc_push_page(vmalloc_block_t *block, addr_t page) {
    *(--block->stack_ptr) = page;
}

/**
 * Pop a page from a block's page stack.
 *
 * Used when allocating a page.
 *
 * The page stacks grows downward: we pre-decrement when pushing pages on the
 * stack and post-increment when popping a page from the stack. The stack
 * pointer points to the next allocatable page.
 *
 * @param block the block whose page stack is to be manipulated
 * @return the page popped from the stack
 *
 * */
static inline addr_t vmalloc_pop_page(vmalloc_block_t *block) {
    return *(block->stack_ptr++);
}

/**
 * Compute the start of a block aligned on block size.
 *
 * @param allocator the kernel address space page allocator
 * @param block_index the index of the block
 * @return the computed address
 *
 * */
static inline addr_t vmalloc_compute_block_base_addr(
        const vmalloc_t     *allocator,
        unsigned int         block_index) {

    return allocator->base_addr + block_index * VMALLOC_BLOCK_SIZE;
}

/**
 * Implementation for vmalloc().
 *
 * @param allocator the allocator from which the page is allocated
 * @return address of allocated page or NULL if allocation failed
 *
 * */
static addr_t vmalloc_from(vmalloc_t *allocator) {
    /** ASSERTION: allocator is not null. */
    assert(allocator != NULL);
    
    vmalloc_block_t *block = allocator->free_list;
    
    if(block == NULL) {
        /* No more page available. */
        return NULL;
    }

    /** ASSERTION: Stack pointer of block is not NULL, which implied that the block has been initialized. */
    assert(block->stack_ptr != NULL);
    
    /** ASSERTION: Since this block is on the free list, the page stack should not be empty. */
    assert(! vmalloc_stack_is_empty(block));
    
    addr_t page = vmalloc_pop_page(block);
    
    /* If we just emptied the stack, unlink the block from the free list. */
    if(vmalloc_stack_is_empty(block)) {
        vmalloc_unlink_block(allocator, block);
    }

    return page;
}

/**
 * Implementation for vmfree().
 *
 * @param allocator the allocator to which the page if returned
 * @param page the address of the page to free
 *
 * */
static void vmfree_to(vmalloc_t *allocator, addr_t page) {
    /** ASSERTION: allocator is not null. */
    assert(allocator != NULL);
    
    /** ASSERTION: We are freeing to the proper allocator/region. */
    assert(addr_is_in_allocator_range(allocator, page));
    
    /** ASSERTION: Page address is page aligned */
    assert(page_offset_of(page) == 0);
    
    /* Find the block to which the free page belongs. */
    unsigned int idx = ( (unsigned int)page - (unsigned int)allocator->base_addr ) / VMALLOC_BLOCK_SIZE;
    vmalloc_block_t *block = &allocator->block_array[idx];
    
    /* If the block was depleted, link it back to the free list. */
    if(vmalloc_stack_is_empty(block)) {
        vmalloc_link_block(allocator, block);
    }
    
    vmalloc_push_page(block, page);
}

/**
 * Implementation for vmalloc_init().
 *
 * @param allocator the initialized allocator.
 * @param start_addr the start address of the region managed by the allocator
 * @param end_addr the end address of the region managed by the allocator
 * @param preinit_limit the limit address for preinitialized blocks
 * @param boot_alloc the initialization-time page allocator structure
 *
 * */
static void init_allocator(
        vmalloc_t       *allocator,
        addr_t           start_addr,
        addr_t           end_addr,
        addr_t           preinit_limit,
        boot_alloc_t    *boot_alloc) {

    addr_t           base_addr;         /* block-aligned start address */
    addr_t           aligned_end;       /* block-aligned end address */
    unsigned int     block_count;       /* array size, in blocks (entries) */
    size_t           array_size;        /* array size, in bytes */
    unsigned int     array_page_count;  /* array size, in pages */
    unsigned int     idx;               /* an array index */
    
    /** ASSERTION: allocator structure pointer must not be null */
    assert(allocator != NULL);
    
    /** ASSERTION: start and end addresses must be multiples of page size (page-aligned memory region) */
    assert( page_offset_of(start_addr) == 0 && page_offset_of(end_addr) == 0 );
    
    /** ASSERTION: the allocation region is non-empty and non-negative */
    assert((uintptr_t)end_addr > (uintptr_t)start_addr);

    /** ASSERTION: the pre-initialized region is within the allocation region and non-empty */
    assert((uintptr_t)preinit_limit > (uintptr_t)start_addr && (uintptr_t)preinit_limit <= (uintptr_t)end_addr);

    /* align base and end addresses to block size */
    base_addr   = ALIGN_START_PTR(start_addr, VMALLOC_BLOCK_SIZE);
    aligned_end = ALIGN_END_PTR(end_addr, VMALLOC_BLOCK_SIZE);

    /* calculate number of memory blocks managed by this allocator */
    block_count = ((char *)aligned_end - (char *)base_addr) / VMALLOC_BLOCK_SIZE;
    
    /* calculate the number of pages required to store the memory block
     * descriptor array */
    array_size              = block_count * sizeof(vmalloc_block_t);
    array_page_count        = ALIGN_END(array_size, PAGE_SIZE) / PAGE_SIZE;
    
    /* initialize allocator struct */
    allocator->start_addr   = start_addr;
    allocator->end_addr     = end_addr;
    allocator->base_addr    = base_addr;
    allocator->block_count  = block_count;
    allocator->free_list    = NULL;
    
    /* allocate block descriptor array pages */
    for(idx = 0; idx < array_page_count; ++idx) {
        /* The kernel panics if this initialization-time page allocation fails
         * so there is no need to check the returned address.
         *
         * The page has to be allocated in the image region because the kernel
         * address space allocator has not yet been initialized. (As I trust you
         * understand, this is what we are doing right now.) */
        addr_t addr = boot_page_alloc_image(boot_alloc);

        if(idx == 0) {
            allocator->block_array = (struct vmalloc_block_t *)addr;
        }

        clear_page(addr);
    }

    /* Initialize pre-initialized blocks */
    vmalloc_initialize_block(
            allocator,
            0,
            boot_page_alloc_image(boot_alloc));

    for(idx = 1; vmalloc_compute_block_base_addr(allocator, idx) < preinit_limit; ++idx) {
        vmalloc_initialize_block(
                allocator,
                idx,
                /* Once the first block has been initialized, we have a fully
                 * functioning implementation of vmalloc(), which allows us to
                 * use boot_page_alloc() instead of boot_page_alloc_image(). */
                boot_page_alloc(boot_alloc));
    }
}

/**
 * Implementation for vmalloc_is_in_range().
 *
 * @param allocator the allocator.
 * @param page the address of the page
 * @return true if it is in the region, false otherwise
 *
 * */
static bool addr_is_in_allocator_range(const vmalloc_t *allocator, addr_t page) {
    return page >= allocator->start_addr && page < allocator->end_addr;
}

/**
 * Link an address space block to the free list.
 *
 * It is not an error if the block is not linked. On exit of this function, the
 * block is in the full state.
 *
 * @param allocator the kernel address space page allocator
 * @param block the block to link from the free list
 *
 * */
static void vmalloc_link_block(vmalloc_t *allocator, vmalloc_block_t *block) {
    if(allocator->free_list == NULL) {
        /* Special case: list is empty. */
        allocator->free_list = block;

        block->next = block;
        block->prev = block;
    }
    else {
        /* The list is circular. The block is added at to the end of the list. */
        vmalloc_block_t *next = allocator->free_list;
        vmalloc_block_t *prev = next->prev;

        block->prev = prev;
        block->next = next;

        prev->next = block;
        next->prev = block;
    }
}

/**
 * Unlink an address space block from the free list.
 *
 * It is not an error if the block is not linked. On exit of this function, the
 * block is in the full state.
 *
 * @param allocator the kernel address space page allocator
 * @param block the block to unlink from the free list
 *
 * */
static void vmalloc_unlink_block(vmalloc_t *allocator, vmalloc_block_t *block) {
    /** ASSERTION: block is not null */
    assert(block != NULL);
    
    /** ASSERTION: block is either properly linked (no null pointers) or not at
     *             all (next is null) */
    assert(block->prev != NULL || block->next == NULL);
    
    /** ASSERTION: block->allocator should not be NULL */
    assert(allocator != NULL);
    
    /* if block is already unlinked, we have nothing to do here */
    if(block->next == NULL) {
        return;
    }

    /* Special case: block is alone in its list.
     *
     * The list is circular. */
    if(block->next == block) {
        /** ASSERTION: if block is alone in its list, the previous node pointer
         *             should point to self */
        assert(block->prev == block);
        
        /** ASSERTION: if block is alone in its list,  we expect it to be the
         *             head of the free list */
        assert(allocator->free_list == block);
        
        allocator->free_list = NULL;
    }
    else {
        if(allocator->free_list == block) {
            allocator->free_list = block->next;
        }
        
        /* unlink block */
        block->next->prev = block->prev;
        block->prev->next = block->next;
    }
    
    /* Set next pointer to NULL to indicate block is unlinked. */
    block->next = NULL;
}

/**
 * Initialize a block's page stack.
 *
 * @param block the block whose page stack is to be initialized
 * @param stack_page the page allocated to be the block's page stack
 * @param start_address the start address of the block
 * @param end_address the end address of the block
 *
 * */
static void vmalloc_stack_init(
        vmalloc_block_t     *block,
        void                *stack_page,
        addr_t               start_address,
        addr_t               end_address) {

    addr_t addr;

    block->stack_base   = stack_page;

    /* The stack grows downward. The stack pointer is pre-decremented when
     * pushing a page. */
    block->stack_ptr    = block->stack_base + VMALLOC_STACK_ENTRIES;

    for(addr = start_address; addr < end_address; addr += PAGE_SIZE) {
        vmalloc_push_page(block, addr);
    }
}

/**
 * Initialize a block so it can be used to allocate address space pages.
 *
 * @param allocator the address space allocator to which the block belongs
 * @param block_index the index of the block to initialize
 * @param stack_page the page allocated to be the block's page stack
 *
 * */
static void vmalloc_initialize_block(vmalloc_t *allocator, unsigned int block_index, void *stack_page) {
    vmalloc_block_t *block = &allocator->block_array[block_index];

    /* The first block may be incomplete, which occurs if the allocator's start
     * address is not aligned to the block size. */
    addr_t start_address;

    if(block_index == 0) {
        start_address = allocator->start_addr;
    }
    else {
        start_address = vmalloc_compute_block_base_addr(allocator, block_index);
    }

    /* Similarly, the last block may be incomplete, which occurs if the
     * allocator's end address is not aligned to the block size. */
    addr_t end_address;

    if(block_index == allocator->block_count - 1) {
        end_address = allocator->end_addr;
    }
    else {
        end_address = vmalloc_compute_block_base_addr(allocator, block_index + 1);
    }

    vmalloc_stack_init(block, stack_page, start_address, end_address);
    vmalloc_link_block(allocator, block);
}
