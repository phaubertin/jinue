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
#include <panic.h>
#include <pfalloc.h>
#include <stddef.h>

pfalloc_cache_t global_pfalloc_cache;

void init_pfalloc_cache(pfalloc_cache_t *pfcache, kern_paddr_t *stack_page) {
    kern_paddr_t    *ptr;
    unsigned int     idx;
    
    ptr = stack_page;
    
    for(idx = 0;idx < KERNEL_PAGE_STACK_SIZE; ++idx) {
        ptr[idx] = PFNULL;
    }
    
    pfcache->ptr   = stack_page;
    pfcache->count = 0;
}

kern_paddr_t pfalloc_from(pfalloc_cache_t *pfcache) {
    if(pfcache->count == 0) {
        panic("pfalloc_from(): no more pages to allocate");
    }    
    
    --pfcache->count;
    
    return *(--pfcache->ptr);
}

void pffree_to(pfalloc_cache_t *pfcache, kern_paddr_t paddr) {
    if(pfcache->count >= KERNEL_PAGE_STACK_SIZE) {
        /** We are leaking memory here. Should we panic instead? */
        return;
    }
    
    ++pfcache->count;
    
    (pfcache->ptr++)[0] = paddr;
}
