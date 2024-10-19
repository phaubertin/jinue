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

#include <kernel/domain/alloc/page_alloc.h>
#include <kernel/domain/alloc/slab.h>
#include <kernel/domain/services/logging.h>
#include <kernel/machine/cpu.h>
#include <kernel/utils/utils.h>
#include <kernel/types.h>
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


/**
 * @file
 *
 * Kernel object allocator
 *
 * This file implements a slab allocator as described in Jeff Bonwick's paper
 * "The Slab Allocator: An Object-Caching Kernel Memory Allocator":
 *
 *  https://www.usenix.org/publications/library/proceedings/bos94/full_papers/bonwick.ps
 *
 * This is the main object allocator for the kernel. (Some early allocations
 * performed during kernel initialization use the boot heap instead - see boot.c.)
 *
 * */

static void init_and_add_slab(slab_cache_t *cache, void *slab_addr);

/**
 * Destroy a slab that is no longer needed.
 *
 * The slab must be free of allocated objects before this function is called. It
 * must also have been unlinked from the free list.
 *
 * This function calls the cache's destructor function, if any, on each free
 * object and then returns the memory to the page allocator.
 *
 * @param cache the cache to which the slab belongs
 * @param slab the slab to destroy.
 *
 * */
static void destroy_slab(slab_cache_t *cache, slab_t *slab) {
    /** ASSERTION: no object is allocated on slab. */
    assert(slab->obj_count == 0);

    addr_t start_addr = ALIGN_START_PTR(slab, SLAB_SIZE);

    /* Call destructor.
     *
     * If the SLAB_POISON flag has been specified when initializing the cache,
     * uninitialized and free objects are filled with recognizable patterns to
     * help detect uninitialized members and writes to freed objects. Obviously,
     * this destroys the constructed state. So, with this debugging feature
     * enabled, the constructor/destructor functions are called when each object
     * is allocated/deallocated instead of when initializing/destroying a slab,
     * i.e. not here. */
    if(cache->dtor != NULL && ! (cache->flags & SLAB_POISON)) {
        for(addr_t buffer = start_addr + slab->colour; buffer < (addr_t)slab; buffer += cache->alloc_size) {
            cache->dtor((void *)buffer, cache->obj_size);
        }
    }
    
    /* return the memory */
    page_free(start_addr);
}

/**
 * Initialize an object cache.
 *
 * The following flags are supported:
 *
 *  - SLAB_HWCACHE_ALIGN Align objects on at least the line size of the CPU's
 *    data cache.
 *  - SLAB_COMPACT the bufctl can safely be put inside the object without
 *    destroying the constructed state. If not set, additional space is reserved
 *    specifically for the bufctl to prevent corruption of the constructed state.
 *  - SLAB_RED_ZONE (redzone checking - debugging) Add a guard word at the end
 *    of each object and use this to detect writes past the end of the object.
 *  - SLAB_POISON (debugging) Fill uninitialized objects with a recognizable
 *    pattern before calling the constructor function to help identify members
 *    that do not get initialized. Do the same when freeing objects and use this
 *    to detect writes to freed objects.
 *
 * This function uses the kernel's boot-time page allocator to allocate an
 * initial slab. This helps with bootstrapping because it allows a few objects
 * (up to s slab's worth) to be allocated before the main page allocator has
 * been initialized and then replenished by user space. It also means this
 * function can only be called during kernel initialization (it would not make
 * sense to call it later).
 *
 * @param cache the cache to initialize
 * @param name a human-readable name for the cache, used in debugging messages
 * @param size the size of objects allocated on this cache
 * @param alignment the minimum object alignment, or zero for no constraint
 * @param ctor the object constructor function
 * @param dtor the object destructor function
 * @param flags see description
 *
 * */
void slab_cache_init(
        slab_cache_t    *cache,
        char            *name,
        size_t           size,
        size_t           alignment,
        slab_ctor_t      ctor,
        slab_ctor_t      dtor,
        int              flags) {

    /** ASSERTION: buffer size is at least the size of a pointer */
    assert(size >= sizeof(void *));
    
    /** ASSERTION: name is not NULL string */
    assert(name != NULL);
    
    cache->name             = name;
    cache->ctor             = ctor;
    cache->dtor             = dtor;
    cache->slabs_empty      = NULL;
    cache->slabs_partial    = NULL;
    cache->slabs_full       = NULL;
    cache->empty_count      = 0;
    cache->flags            = flags;
    cache->next_colour      = 0;
    cache->working_set      = SLAB_DEFAULT_WORKING_SET;
    
    /* Compute actual alignment. */
    if(alignment == 0) {
        cache->alignment = sizeof(uint32_t);
    }
    else {
        cache->alignment = alignment;
    }

    unsigned int dcache_alignment = machine_get_cpu_dcache_alignment();
    
    if((flags & SLAB_HWCACHE_ALIGN) && cache->alignment < dcache_alignment) {
        cache->alignment = dcache_alignment;
    }
    
    cache->alignment = ALIGN_END(cache->alignment, sizeof(uint32_t));
    
    /* Reserve space for bufctl and/or redzone word. */
    cache->obj_size = ALIGN_END(size, sizeof(uint32_t));
    
    if((flags & SLAB_POISON) && (flags & SLAB_RED_ZONE)) {
        /* bufctl and redzone word appended to buffer */
        cache->alloc_size = cache->obj_size + sizeof(uint32_t) + sizeof(slab_bufctl_t);
    }
    else if((flags & SLAB_POISON) || (flags & SLAB_RED_ZONE)) {
        /* bufctl or redzone word appended to buffer (can be shared) */
        cache->alloc_size = cache->obj_size + sizeof(uint32_t);
    }
    else if(ctor != NULL && ! (flags & SLAB_COMPACT)) {
        /* If a constructor is defined, we cannot put the bufctl inside
         * the object because that could overwrite constructed state,
         * unless client explicitly says it's ok (SLAB_COMPACT flag). */
        cache->alloc_size = cache->obj_size + sizeof(slab_bufctl_t);
    }
    else {
        cache->alloc_size = cache->obj_size;
    }
    
    if(cache->alloc_size % cache->alignment != 0) {
        cache->alloc_size += cache->alignment - cache->alloc_size % cache->alignment;
    }
    
    size_t avail_space = SLAB_SIZE - sizeof(slab_t);
        
    unsigned int buffers_per_slab = avail_space / cache->alloc_size;
    
    size_t wasted_space = avail_space - buffers_per_slab * cache->alloc_size;
    
    cache->max_colour = (wasted_space / cache->alignment) * cache->alignment;
    
    cache->bufctl_offset = cache->alloc_size - sizeof(slab_bufctl_t);

    /* Allocate first slab.
     *
     * This is needed to allow a few objects to be allocated during kernel
     * initialization. */
    init_and_add_slab(cache, page_alloc());
}

/**
 * Allocate an object from the specified cache.
 *
 * The cache must have been initialized with slab_cache_init(). If no more space
 * is available on existing slabs, this function tries to allocate a new slab
 * using the kernel's page allocator (i.e. page_alloc()). It page allocation
 * fails, this function fails by returning NULL.
 *
 * @param cache the cache from which to allocate an object
 * @return the address of the allocated object, or NULL if allocation failed
 *
 * */
void *slab_cache_alloc(slab_cache_t *cache) {
    slab_t          *slab;
    
    if(cache->slabs_partial != NULL) {
        slab = cache->slabs_partial;
    }
    else {
        if(cache->slabs_empty == NULL) {
            void *slab_addr = page_alloc();

            if(slab_addr == NULL) {
                return NULL;
            }

            init_and_add_slab(cache, slab_addr);
        }
        
        slab = cache->slabs_empty;
        
        /** ASSERTION: now that slab_cache_grow() has run, we should have found at least one empty slab */
        assert(slab != NULL);        
        
        /** 
         * Important note regarding the slab lists:
         *  The empty, partial and full slab lists are doubly-linked lists.
         *  This is done to allow the deletion of an arbitrary link given
         *  a pointer to it. We do not allow reverse traversal: we do not
         *  maintain a tail pointer and, more importantly: we do _NOT_
         *  maintain the previous pointer of the first link in the list
         *  (i.e. it is garbage data, not NULL). */
        
        /* We are about to allocate one object from this slab, so it will
         *  not be empty anymore...*/        
        cache->slabs_empty      = slab->next;
        
        --(cache->empty_count);
        
        slab->next              = cache->slabs_partial;
        if(slab->next != NULL) {
            slab->next->prev = slab;
        }
        cache->slabs_partial    = slab;
    }
    
    slab_bufctl_t *bufctl = slab->free_list;
    
    /** ASSERTION: there is at least one buffer on the free list */
    assert(bufctl != NULL);
    
    slab->free_list  = bufctl->next;
    slab->obj_count += 1;
    
    /* If we just allocated the last buffer, move the slab to the full
     * list */
    if(slab->free_list == NULL) {
        /* remove from the partial slabs list */
        
        /** ASSERT: the slab is the head of the partial list */
        assert(cache->slabs_partial == slab);
        
        cache->slabs_partial = slab->next;
       
        if(slab->next != NULL) {
            slab->next->prev = slab->prev;
        }
        
        /* add to the full slabs list */
        slab->next        = cache->slabs_full;
        cache->slabs_full = slab;
        
        if(slab->next != NULL) {
            slab->next->prev = slab;
        }
    }
    
    uint32_t *buffer = (uint32_t *)( (char *)bufctl - cache->bufctl_offset );
    
    if(cache->flags & SLAB_POISON) {
        unsigned int idx;
        unsigned int dump_lines = 0;
        
        for(idx = 0; idx < cache->obj_size / sizeof(uint32_t); ++idx) {
            if(buffer[idx] != SLAB_POISON_DEAD_VALUE) {
                if(dump_lines == 0) {
                    warning("detected write to freed object, cache: %s buffer: %#p:",
                        cache->name,
                        buffer
                    );
                }
                
                if(dump_lines < 4) {
                    warning("  value %#x at byte offset %u", buffer[idx], idx * sizeof(uint32_t));
                }
                
                ++dump_lines;
            }
            
            buffer[idx] = SLAB_POISON_ALIVE_VALUE;
        }
        
        /* If both SLAB_POISON and SLAB_RED_ZONE are enabled, we perform
         * redzone checking even on freed objects. */
        if(cache->flags & SLAB_RED_ZONE) {
            if(buffer[idx] != SLAB_RED_ZONE_VALUE) {
                warning("detected write past the end of freed object, cache: %s buffer: %#p value: %#" PRIx32,
                    cache->name,
                    buffer,
                    buffer[idx]
                );
            }
            
            buffer[idx] = SLAB_RED_ZONE_VALUE;
        }
        
        if(cache->ctor != NULL) {
            cache->ctor((void *)buffer, cache->obj_size);
        }
    }
    else if(cache->flags & SLAB_RED_ZONE) {
        buffer[cache->obj_size / sizeof(uint32_t)] = SLAB_RED_ZONE_VALUE;
    }
        
    return (void *)buffer;
}

/**
 * Free an object.
 *
 * @param buffer the object to free
 *
 * */
void slab_cache_free(void *buffer) {
    /* compute address of slab data structure */
    addr_t slab_start       = ALIGN_START_PTR(buffer, SLAB_SIZE);
    slab_t *slab            = (slab_t *)(slab_start + SLAB_SIZE - sizeof(slab_t) );
    
    /* obtain address of cache and bufctl */
    slab_cache_t *cache     = slab->cache;
    slab_bufctl_t *bufctl   = (slab_bufctl_t *)((char *)buffer + cache->bufctl_offset);
    
    /* If slab is on the full slabs list, move it to the partial list
     * since we are about to return a buffer to it. */
    if(slab->free_list == NULL) {
        /* remove from full slabs list */
        if(cache->slabs_full == slab) {
            cache->slabs_full = slab->next;
        }
        else {
            slab->prev->next = slab->next;
        }
        
        if(slab->next != NULL) {
            slab->next->prev = slab->prev;
        }
        
        /* add to partial slabs list */
        slab->next           = cache->slabs_partial;
        cache->slabs_partial = slab;
        
        if(slab->next != NULL) {
            slab->next->prev = slab;
        }
    }
    
    if(cache->flags & SLAB_RED_ZONE) {
        uint32_t *rz_word = (uint32_t *)((char *)buffer + cache->obj_size);
        
        if(*rz_word != SLAB_RED_ZONE_VALUE) {
            warning("detected write past the end of object, cache: %s buffer: %#p value: %#" PRIx32,
                cache->name,
                buffer,
                *rz_word
            );
        }
        
        *rz_word = SLAB_RED_ZONE_VALUE;
    }
    
    if(cache->flags & SLAB_POISON) {
        unsigned int idx;

        if(cache->dtor != NULL) {
            cache->dtor(buffer, cache->obj_size);
        }
        
        uint32_t *buffer32 = (uint32_t *)buffer;
        
        for(idx = 0; idx < cache->obj_size / sizeof(uint32_t); ++idx) {
            buffer32[idx] = SLAB_POISON_DEAD_VALUE;
        }
    }
    
    /* link buffer into slab free list */
    bufctl->next     = slab->free_list;
    slab->free_list  = bufctl;
    slab->obj_count -= 1;
    
    /* If we just returned the last object to the slab, move the slab to
     * the empty list. */
    if(slab->obj_count == 0) {
        /* remove from partial slabs list */
        if(cache->slabs_partial == slab) {
            cache->slabs_partial = slab->next;
        }
        else {
            slab->prev->next = slab->next;
        }
        
        if(slab->next != NULL) {
            slab->next->prev = slab->prev;
        }
        
        /* add to empty slabs list */
        slab->next         = cache->slabs_empty;
        cache->slabs_empty = slab;
        
        if(slab->next != NULL) {
            slab->next->prev = slab;
        }
        
        ++(cache->empty_count);
    }     
}

/**
 * Initialize a new empty slab and add it to a cache's free list.
 *
 * This function will not fail because the page of memory to be used for the
 * slab is allocated by the caller and a pointer to it is passed as an argument.
 * This page can be allocated from either the kernel's main page allocator or
 * from the boot-time page allocator.
 *
 * @param cache the cache to which a slab is added
 * @param slab_addr an appropriately allocated page for use as new slab
 *
 * */
static void init_and_add_slab(slab_cache_t *cache, void *slab_addr) {
    /** ASSERTION: slab address is not NULL */
    assert(slab_addr != NULL);
    
    slab_t *slab = (slab_t *)((char *)slab_addr + SLAB_SIZE - sizeof(slab_t));

    slab->cache = cache;
    
    /* slab is initially empty */
    slab->obj_count = 0;
    
    slab->next         = cache->slabs_empty;
    cache->slabs_empty = slab;
    
    if(slab->next != NULL) {
        slab->next->prev = slab;
    }
    
    ++(cache->empty_count);
    
    /* set slab colour and update cache next colour */
    slab->colour = cache->next_colour;
    
    if(cache->next_colour < cache->max_colour) {
        cache->next_colour += cache->alignment;
    }
    else {
        cache->next_colour = 0;
    }
    
    /* compute address of first bufctl */
    slab_bufctl_t *bufctl   = (slab_bufctl_t *)((char *)slab_addr + slab->colour + cache->bufctl_offset);
    slab->free_list         = bufctl;
    
    while(true) {
        addr_t buffer = (addr_t)bufctl - cache->bufctl_offset;
        
        if(cache->flags & SLAB_POISON) {
            uint32_t *ptr;
            uint32_t *buffer_end = (uint32_t *)(buffer + cache->obj_size);
            
            for(ptr = (uint32_t *)buffer; ptr < buffer_end; ++ptr) {
                *ptr = SLAB_POISON_DEAD_VALUE;
            }
            
            /* If both SLAB_POISON and SLAB_RED_ZONE are enabled, we
             * perform redzone checking even on freed objects. */
            if(cache->flags & SLAB_RED_ZONE) {
                *ptr = SLAB_RED_ZONE_VALUE;
            }
        }
        else if (cache->ctor != NULL) {
            cache->ctor((void *)buffer, cache->obj_size);
        }
        
        slab_bufctl_t *next = (slab_bufctl_t *)((char *)bufctl + cache->alloc_size);
        
        /** TODO: check this condition */
        if(next >= (slab_bufctl_t *)slab) {
            bufctl->next = NULL;
            break;
        }
        
        bufctl->next = next;
        bufctl = next;
    }
}

/**
 * Return memory to the page allocator.
 *
 * Free slabs in excess to the cache's working set are finalized and freed.
 *
 * @param cache the cache from which to reclaim memory
 *
 * */
void slab_cache_reap(slab_cache_t *cache) {
    while(cache->empty_count > cache->working_set) {
        /* select the first empty slab */
        slab_t *slab = cache->slabs_empty;
        
        /* unlink it and update count */
        cache->slabs_empty  = slab->next;
        cache->empty_count -= 1;
        
        /* destroy slab */
        destroy_slab(cache, slab);
    }
}

/**
 * Set a cache's working set.
 *
 * The working set is defined as the number of free slabs the cache keeps for
 * itself when pages are reclaimed from it. (This is terminology used in the
 * Bonwick paper.) This provides some hysteresis to prevent slabs from being
 * continuously created and destroyed, which requires calling the constructor
 * and destructor functions on individual objects on the slabs.
 *
 * @param cache the cache for which to set the working set
 * @param n the size of the working set (number of pages)
 *
 * */
void slab_cache_set_working_set(slab_cache_t *cache, unsigned int n) {
    cache->working_set = n;
}
