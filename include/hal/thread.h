#ifndef JINUE_HAL_THREAD_H
#define JINUE_HAL_THREAD_H

#include <hal/asm/thread.h>

#include <jinue/types.h>
#include <hal/thread_ctx.h>
#include <hal/vm.h>
#include <hal/x86.h>
#include <stddef.h>
#include <stdbool.h>
#include <thread_decl.h>

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
        addr_space_t    *addr_space,
        addr_t           entry,
        addr_t           user_stack);

void thread_page_destroy(thread_t *thread) ;

void thread_context_switch(
        thread_context_t    *from_ctx,
        thread_context_t    *to_ctx,
        bool                 destroy_from);

#endif
