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

#ifndef JINUE_KERNEL_VM_ALLOC_H
#define JINUE_KERNEL_VM_ALLOC_H

#include <types.h>


#define VM_ALLOC_STACK_ENTRIES    1024

#define VM_ALLOC_BLOCK_SIZE        (VM_ALLOC_STACK_ENTRIES * PAGE_SIZE)

#define VM_ALLOC_BLOCK_MASK        (VM_ALLOC_BLOCK_SIZE - 1)


#define VM_ALLOC_WAS_FREE        false

#define VM_ALLOC_WAS_USED        true


#define VM_ALLOC_IS_FREE(b)        ( (b)->next != NULL && (b)->stack_ptr == NULL )

#define VM_ALLOC_IS_PARTIAL(b)    ( (b)->next != NULL && (b)->stack_ptr != NULL )

#define VM_ALLOC_IS_USED(b)        ( (b)->next == NULL )


#define VM_ALLOC_EMPTY_STACK(b)    ( (b)->stack_ptr >= (b)->stack_addr + VM_ALLOC_STACK_ENTRIES )

#define VM_ALLOC_FULL_STACK(b)    ( (b)->stack_ptr <= (b)->stack_addr + 1)

#define VM_ALLOC_CANNOT_GROW(b)    ( (b)->stack_next >= (b)->base_addr + VM_ALLOC_BLOCK_SIZE )


struct vm_alloc_t {
    /** base address of memory managed by allocator */
    addr_t base_addr;
    
    /** start address of memory actually available to the allocator */
    addr_t start_addr;
    
    /** end address of memory actually available to the allocator */
    addr_t end_addr;

    /** number of memory blocks managed by this allocator */
    unsigned int block_count;
    
    /** array of memory block descriptors */
    struct vm_block_t *block_array;
        
    /** number of pages allocated for block array */
    unsigned int array_pages;
    
    /** list of completely free blocks */
    struct vm_block_t *free_list;
    
    /** list of partially free blocks */
    struct vm_block_t *partial_list;
};

struct vm_block_t {
    /** base address of memory block */
    addr_t base_addr;
    
    /** allocator to which this block belongs */
    struct vm_alloc_t *allocator;
    
    /** stack pointer for stack of free pages in partially allocated blocks */
    addr_t *stack_ptr;
    
    /** base address of free page stack */
    addr_t *stack_addr;
    
    /** next page address to add to stack, used for deferred stack initialization */
    addr_t stack_next;
    
    /** link previous block in free list */
    struct vm_block_t *prev;
    
    /** link next block in free list */
    struct vm_block_t *next;
};


typedef struct vm_alloc_t vm_alloc_t;

typedef struct vm_block_t vm_block_t;


extern vm_alloc_t *global_page_allocator;


addr_t vm_alloc(vm_alloc_t *allocator);

addr_t vm_alloc_low_latency(vm_alloc_t *allocator);

void vm_free(vm_alloc_t *allocator, addr_t page);

void vm_alloc_init(vm_alloc_t *allocator, addr_t start_addr, addr_t end_addr);

void vm_alloc_destroy(vm_alloc_t *allocator);

void vm_alloc_init_allocator(vm_alloc_t *allocator, addr_t start_addr, addr_t end_addr);

void vm_alloc_add_region(vm_alloc_t *allocator, addr_t start_addr, addr_t end_addr);

void vm_alloc_free_block(vm_block_t *block);

void vm_alloc_partial_block(vm_block_t *block);

void vm_alloc_custom_block(vm_block_t *block, addr_t start_addr, addr_t end_addr);

void vm_alloc_unlink_block(vm_block_t *block);

void vm_alloc_grow_stack(vm_block_t *block);

addr_t vm_alloc_grow_single(vm_block_t *block);

#endif
