#ifndef _JINUE_KERNEL_HAL_THREAD_H_
#define _JINUE_KERNEL_HAL_THREAD_H_

#include <jinue/types.h>
#include <hal/vm.h>
#include <stddef.h>

#include <hal/asm/thread.h>


/* For each thread context, a block is allocated that contains the thread
 * context structure (thread_context_t), followed by the kernel stack for that
 * thread context. Switching thread context (see thread_context_switch())
 * involves mostly switching the kernel stack.
 *
 *  +--------v-----------------v--------+ thread_ctx + THREAD_CONTEXT_SIZE
 *  |                                   |
 *  |                                   |
 *  |                                   |
 *  |            Kernel stack           |
 *  |                                   |
 *  |                                   |
 *  |                                   |
 *  |                                   |
 *  +-----------------------------------+ thread_ctx + sizeof(thread_context_t)
 *  |                                   |
 *  |     Thread context structure      |
 *  |        (thread_context_t)         |
 *  |                                   |
 *  +-----------------------------------+ thread_ctx
 *
 * The thread context block size (THREAD_CONTEXT_SIZE) is a power of two and
 * each block is allocated aligned on this size. Because of this, the current
 * thread context structure can be found by masking the least significant bits
 * of the stack pointer (with THREAD_CONTEXT_MASK). */

struct thread_context_t {
    /* The assembly language thread switching code makes the assumption that
     * saved_stack_pointer is the first member of this struct. */
    addr_t           saved_stack_pointer;
    addr_space_t    *addr_space;
    addr_t           local_storage_addr;
    size_t           local_storage_size;
};

typedef struct thread_context_t thread_context_t;


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
    return (addr_t)thread_ctx + THREAD_CONTEXT_SIZE;
}

void thread_context_init_cache(void);

thread_context_t *thread_context_create(
        addr_space_t    *addr_space,
        addr_t           entry,
        addr_t           user_stack);

void thread_context_switch(thread_context_t *thread_ctx);

#endif
