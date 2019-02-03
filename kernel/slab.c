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

#include <hal/cpu.h>
#include <hal/pfaddr.h>
#include <hal/vm.h>
#include <assert.h>
#include <pfalloc.h>
#include <printk.h>
#include <slab.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>
#include <util.h>
#include <vm_alloc.h>

slab_cache_t *slab_cache_list = NULL;

static void destroy_slab(slab_cache_t *cache, slab_t *slab) {
    addr_t           start_addr;
    addr_t           buffer;
    pfaddr_t         paddr;

    /* call destructor */
    start_addr = ALIGN_START(slab, SLAB_SIZE);
    
    for(buffer = start_addr + slab->colour; buffer < (addr_t)slab; buffer += cache->alloc_size) {
        if(cache->dtor != NULL && ! (cache->flags & SLAB_POISON)) {
            cache->dtor((void *)buffer, cache->obj_size);
        }
    }
    
    /* return the memory */
    paddr = vm_lookup_pfaddr(NULL, start_addr);
    vm_unmap_kernel(start_addr);
    vm_free(global_page_allocator, start_addr);
    pffree(paddr);
}

void slab_cache_init(
        slab_cache_t    *cache,
        char            *name,
        size_t           size,
        size_t           alignment,
        slab_ctor_t      ctor,
        slab_ctor_t      dtor,
        int              flags) {
    
    size_t           avail_space;
    size_t           wasted_space;
    unsigned int     buffers_per_slab;

    /** ASSERTION: ensure buffer size is at least the size of a pointer */
    assert( size >= sizeof(void *) );
    
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
    
    /* add new cache to cache list */
    cache->next             = slab_cache_list;
    slab_cache_list         = cache;
    
    if(cache->next != NULL) {
        cache->next->prev = cache;
    }
    
    /* compute actual alignment */
    if(alignment == 0) {
        cache->alignment = sizeof(uint32_t);
    }
    else {
        cache->alignment = alignment;
    }
    
    if((flags & SLAB_HWCACHE_ALIGN) && cache->alignment < cpu_info.dcache_alignment) {
        cache->alignment = cpu_info.dcache_alignment;
    }
    
    if(cache->alignment % sizeof(uint32_t) != 0) {
        cache->alignment += sizeof(uint32_t) - cache->alignment % sizeof(uint32_t);
    }
    
    /* reserve space for bufctl and/or redzone word */
    cache->obj_size = size;
    
    if(cache->obj_size % sizeof(uint32_t) != 0) {
        cache->obj_size += sizeof(uint32_t) - cache->obj_size % sizeof(uint32_t);
    }
    
    if((flags & SLAB_POISON) && (flags & SLAB_RED_ZONE)) {
        /* bufctl and redzone word appended to buffer */
        cache->alloc_size = cache->obj_size + sizeof(uint32_t) + sizeof(slab_bufctl_t);
    }
    else if((flags & SLAB_POISON) || (flags & SLAB_RED_ZONE)) {
        /* bufctl and/or redzone word appended to buffer
         * (can be shared) */
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
    
    avail_space = SLAB_SIZE - sizeof(slab_t);
        
    buffers_per_slab = avail_space / cache->alloc_size;
    
    wasted_space = avail_space - buffers_per_slab * cache->alloc_size;
    
    cache->max_colour = (wasted_space / cache->alignment) * cache->alignment;
    
    cache->bufctl_offset = cache->alloc_size - sizeof(slab_bufctl_t);
}

void *slab_cache_alloc(slab_cache_t *cache) {
    slab_t          *slab;
    slab_bufctl_t   *bufctl;
    uint32_t        *buffer;
    unsigned int     idx;
    unsigned int     dump_lines;
    
    if(cache->slabs_partial != NULL) {
        slab = cache->slabs_partial;
    }
    else {
        if(cache->slabs_empty == NULL) {
            slab_cache_grow(cache);
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
    
    bufctl = slab->free_list;
    
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
    
    buffer = (uint32_t *)( (char *)bufctl - cache->bufctl_offset );
    
    if(cache->flags & SLAB_POISON) {
        dump_lines = 0;
        
        for(idx = 0; idx < cache->obj_size / sizeof(uint32_t); ++idx) {
            if(buffer[idx] != SLAB_POISON_DEAD_VALUE) {
                if(dump_lines == 0) {
                    printk("detected write to freed object, cache: %s buffer: 0x%x:\n",
                        cache->name,
                        (unsigned int)buffer
                    );
                }
                
                if(dump_lines < 4) {
                    printk(" value 0x%x at byte offset %u\n", buffer[idx], idx * sizeof(uint32_t));
                }
                
                ++dump_lines;
            }
            
            buffer[idx] = SLAB_POISON_ALIVE_VALUE;
        }
        
        /* If both SLAB_POISON and SLAB_RED_ZONE are enabled, we perform
         * redzone checking even on freed objects. */
        if(cache->flags & SLAB_RED_ZONE) {
            if(buffer[idx] != SLAB_RED_ZONE_VALUE) {
                printk("detected write past the end of freed object, cache: %s buffer: 0x%x value: 0x%x\n",
                    cache->name,
                    (unsigned int)buffer,
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

void slab_cache_free(void *buffer) {
    addr_t           slab_start;
    slab_t          *slab;
    slab_cache_t    *cache;
    slab_bufctl_t   *bufctl;
    uint32_t        *rz_word;
    uint32_t        *buffer32;
    unsigned int     idx;
    
    /* compute address of slab data structure */
    slab_start = ALIGN_START(buffer, SLAB_SIZE);
    slab = (slab_t *)(slab_start + SLAB_SIZE - sizeof(slab_t) );
    
    /* obtain address of cache and bufctl */
    cache  = slab->cache;
    bufctl = (slab_bufctl_t *)((char *)buffer + cache->bufctl_offset);
    
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
        rz_word = (uint32_t *)( (char *)buffer + cache->obj_size );
        
        if(*rz_word != SLAB_RED_ZONE_VALUE) {
            printk("detected write past the end of object, cache: %s buffer: 0x%x value: 0x%x\n",
                cache->name,
                (unsigned int)buffer,
                *rz_word
            );
        }
        
        *rz_word = SLAB_RED_ZONE_VALUE;
    }
    
    if(cache->flags & SLAB_POISON) {
        if(cache->dtor != NULL) {
            cache->dtor(buffer, cache->obj_size);
        }
        
        buffer32 = (uint32_t *)buffer;
        
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

void slab_cache_grow(slab_cache_t *cache) {
    void            *slab_addr;
    slab_t          *slab;
    slab_bufctl_t   *bufctl;
    slab_bufctl_t   *next;
    addr_t           buffer;
    uint32_t        *buffer_end;
    uint32_t        *ptr;
    
    /* allocate new slab */
    slab_addr = vm_alloc( global_page_allocator );
    
    /** ASSERTION: slab address is not NULL */
    assert(slab_addr != NULL);
    
    vm_map_kernel(slab_addr, pfalloc(), VM_FLAG_READ_WRITE);
    
    slab = (slab_t *)( (char *)slab_addr + SLAB_SIZE - sizeof(slab_t) );
    
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
    bufctl          = (slab_bufctl_t *)( (char *)slab_addr + slab->colour + cache->bufctl_offset );
    slab->free_list = bufctl;
    
    while(1) {        
        buffer = (addr_t)bufctl - cache->bufctl_offset;
        
        if(cache->flags & SLAB_POISON) {
            buffer_end = (uint32_t *)(buffer + cache->obj_size);
            
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
        
        next = (slab_bufctl_t *)( (char *)bufctl + cache->alloc_size );
        
        /** TODO: check this condition */
        if(next >= (slab_bufctl_t *)slab) {
            bufctl->next = NULL;
            break;
        }
        
        bufctl->next = next;
        bufctl = next;
    }
}

void slab_cache_reap(slab_cache_t *cache) {
    slab_t          *slab;
    
    while(cache->empty_count > cache->working_set) {
        /* select the first empty slab */
        slab = cache->slabs_empty;
        
        /* unlink it and update count */
        cache->slabs_empty  = slab->next;
        cache->empty_count -= 1;
        
        /* destroy slab */
        destroy_slab(cache, slab);
    }
}

void slab_cache_set_working_set(slab_cache_t *cache, unsigned int n) {
    cache->working_set = n;
}
