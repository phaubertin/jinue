#ifndef _JINUE_KERNEL_THREAD_H_
#define _JINUE_KERNEL_THREAD_H_

#include <hal/thread.h>
#include <thread_decl.h>

static inline void thread_switch(thread_t *from_thread, thread_t *to_thread) {
    thread_context_switch(
        &from_thread->thread_ctx,
        &to_thread->thread_ctx );
}

thread_t *thread_create(
        addr_space_t    *addr_space,
        addr_t           entry,
        addr_t           user_stack);

#endif
