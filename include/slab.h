/*
 * Copyright (C) 2017 Philippe Aubertin.
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

#ifndef JINUE_KERNEL_SLAB_H
#define JINUE_KERNEL_SLAB_H

#include <jinue-common/vm.h>
#include <stddef.h>

#define SLAB_SIZE                   PAGE_SIZE

#define SLAB_POISON_ALIVE_VALUE     0x0BADCAFE

#define SLAB_POISON_DEAD_VALUE      0xDEADBEEF

#define SLAB_RED_ZONE_VALUE         0x5711600D

#define SLAB_DEFAULT_WORKING_SET    2


#define SLAB_DEFAULTS               (0)

#define SLAB_RED_ZONE               (1<<0)

#define SLAB_POISON                 (1<<1)

#define SLAB_HWCACHE_ALIGN          (1<<2)

#define SLAB_COMPACT                (1<<3)


typedef void (*slab_ctor_t)(void *, size_t);

struct slab_t;

struct slab_cache_t {
    struct slab_t       *slabs_empty;
    struct slab_t       *slabs_partial;
    struct slab_t       *slabs_full;
    unsigned int         empty_count;
    size_t               obj_size;
    size_t               alloc_size;
    size_t               alignment;
    size_t               bufctl_offset;
    size_t               next_colour;
    size_t               max_colour;
    unsigned int         working_set;
    slab_ctor_t          ctor;
    slab_ctor_t          dtor;
    char                *name;
    struct slab_cache_t *prev;
    struct slab_cache_t *next;
    int                  flags;
};

typedef struct slab_cache_t slab_cache_t;

struct slab_bufctl_t {
    struct slab_bufctl_t *next;
};

typedef struct slab_bufctl_t slab_bufctl_t;

struct slab_t {
    struct slab_t   *prev;
    struct slab_t   *next;
    
    slab_cache_t    *cache;
    
    unsigned int     obj_count;
    size_t           colour;
    slab_bufctl_t   *free_list;
};

typedef struct slab_t slab_t;

extern slab_cache_t *slab_cache_list;


slab_cache_t *slab_cache_create(
    char            *name,
    size_t           size,
    size_t           alignment,
    slab_ctor_t      ctor,
    slab_ctor_t      dtor,
    int              flags );

void slab_cache_destroy(slab_cache_t *cache);

void *slab_cache_alloc(slab_cache_t *cache);

void slab_cache_free(void *buffer);

void slab_cache_grow(slab_cache_t *cache);

void slab_cache_reap(slab_cache_t *cache);

void slab_cache_set_working_set(slab_cache_t *cache, unsigned int n);

#endif
