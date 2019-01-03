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

#ifndef JINUE_HAL_THREAD_H
#define JINUE_HAL_THREAD_H

#include <hal/asm/thread.h>

#include <hal/x86.h>
#include <types.h>


static inline thread_t *get_current_thread(void) {
    return (thread_t *)(get_esp() & THREAD_CONTEXT_MASK);
}

static inline void thread_context_set_local_storage(
        thread_context_t    *thread_ctx,
        addr_t               addr,
        size_t               size) {

    thread_ctx->local_storage_addr  = addr;
    thread_ctx->local_storage_size  = size;
}

static inline addr_t thread_context_get_local_storage(thread_context_t *thread_ctx) {
    return thread_ctx->local_storage_addr;
}

static inline addr_t get_kernel_stack_base(thread_context_t *thread_ctx) {
    thread_t *thread = (thread_t *)( (uintptr_t)thread_ctx & THREAD_CONTEXT_MASK );
    
    return (addr_t)thread + THREAD_CONTEXT_SIZE;
}

thread_t *thread_page_create(
        addr_t           entry,
        addr_t           user_stack);

void thread_page_destroy(thread_t *thread) ;

void thread_context_switch(
        thread_context_t    *from_ctx,
        thread_context_t    *to_ctx,
        bool                 destroy_from);

#endif
