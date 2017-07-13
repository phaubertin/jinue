#ifndef JINUE_KERNEL_THREAD_H
#define JINUE_KERNEL_THREAD_H

#include <hal/thread.h>
#include <stdbool.h>
#include <thread_decl.h>

thread_t *thread_create(
        addr_space_t    *addr_space,
        addr_t           entry,
        addr_t           user_stack);
        
void thread_ready(thread_t *thread);

void thread_switch(
        thread_t *from_thread,
        thread_t *to_thread,
        bool blocked,
        bool do_destroy);

void thread_yield_from(thread_t *from_thread, bool blocked, bool do_destroy);

#endif
