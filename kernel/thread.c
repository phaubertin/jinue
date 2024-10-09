/*
 * Copyright (C) 2019-2022 Philippe Aubertin.
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

#include <jinue/shared/asm/errno.h>
#include <kernel/i686/cpu_data.h>
#include <kernel/i686/thread.h>
#include <kernel/i686/vm.h>
#include <kernel/boot.h>
#include <kernel/descriptor.h>
#include <kernel/list.h>
#include <kernel/object.h>
#include <kernel/process.h>
#include <kernel/page_alloc.h>
#include <kernel/panic.h>
#include <kernel/thread.h>

/** runtime type definition for a thread */
static const object_type_t object_type = {
    .all_permissions    = 0,
    .name               = "thread",
    .size               = sizeof(thread_t),
    .open               = NULL,
    .close              = NULL,
    .cache_ctor         = NULL,
    .cache_dtor         = NULL
};

const object_type_t *object_type_thread = &object_type;

static jinue_list_t ready_list = JINUE_LIST_STATIC;

int thread_create_syscall(
        int              process_fd,
        void            *entry,
        void            *user_stack) {
    object_header_t *object;

    int status = dereference_object_descriptor(
            &object,
            NULL,
            get_current_thread()->process,
            process_fd
    );

    if(status < 0) {
        return status;
    }

    if(object->type != object_type_process) {
        return -JINUE_EBADF;
    }

    process_t *process = (process_t *)object;
    const thread_t *thread = thread_create(process, entry, user_stack);

    return (thread == NULL) ? -JINUE_EAGAIN : 0;
}

static void thread_init(thread_t *thread, process_t *process) {
    object_header_init(&thread->header, object_type_thread);

    jinue_node_init(&thread->thread_list);

    thread->process             = process;
    thread->sender              = NULL;
    thread->local_storage_addr  = NULL;
    thread->local_storage_size  = 0;

    thread_ready(thread);
}

thread_t *thread_create(
        process_t       *process,
        void            *entry,
        void            *user_stack) {

    void *thread_page = page_alloc();

    if(thread_page == NULL) {
        return NULL;
    }

    thread_t *thread = thread_page_init(thread_page, entry, user_stack);
    thread_init(thread, process);
    
    return thread;
}

/* This function is called by assembled code. See thread_context_switch_stack(). */
void thread_destroy(thread_t *thread) {
    void * thread_page = (void *)((uintptr_t)thread & ~PAGE_MASK);
    page_free(thread_page);
}

void thread_ready(thread_t *thread) {
    /* add thread to the tail of the ready list to give other threads a chance
     * to run */
    jinue_list_enqueue(&ready_list, &thread->thread_list);
}

static void switch_thread(
        thread_t    *from_thread,
        thread_t    *to_thread,
        bool         do_destroy) {

    if(to_thread != from_thread) {
        thread_context_t    *from_context;
        process_t           *from_process;

        if(from_thread == NULL) {
            from_context = NULL;
            from_process = NULL;
        }
        else {
            from_context = &from_thread->thread_ctx;
            from_process = from_thread->process;
        }

        if(from_process != to_thread->process) {
            process_switch_to(to_thread->process);
        }

        thread_context_switch(
            from_context,
            &to_thread->thread_ctx,
            do_destroy);
    }
}

static thread_t *reschedule(bool current_can_run) {
    thread_t *to_thread = jinue_node_entry(
            jinue_list_dequeue(&ready_list),
            thread_t,
            thread_list );
    
    if(to_thread == NULL) {
        /* Special case to take into account: when scheduling the first thread,
         * there is no current thread. We should not call get_current_thread()
         * in that case. */
        if(current_can_run) {
            return get_current_thread();
        }

        /* Currently, scheduling is purely cooperative and only one CPU is
         * supported (so, there are no threads currently running on other
         * CPUs). What this means is that, once there are no more threads
         * running or ready to run, this situation will never change. */
        panic("No thread to schedule");
    }

    return to_thread;
}

void thread_switch_to(thread_t *thread, bool blocked) {
    thread_t *current = get_current_thread();

    if (!blocked) {
        thread_ready(current);
    }

    switch_thread(
            current,
            thread,
            false);             /* don't destroy current thread */
}

void thread_start_first(void) {
    switch_thread(
            NULL,
            reschedule(false),
            false);             /* don't destroy current thread */
}

void thread_yield(void) {
    thread_switch_to(
            reschedule(true),   /* current thread can run */
            false);             /* don't block current thread */
}

void thread_block(void) {
    thread_switch_to(
            reschedule(false),  /* current thread cannot run */
            true);              /* do block current thread */
}

void thread_exit(void) {
    switch_thread(
            get_current_thread(),
            reschedule(false),
            true);              /* do destroy the thread */
}

void thread_set_local_storage(
        thread_t    *thread,
        addr_t       addr,
        size_t       size) {

    thread->local_storage_addr  = addr;
    thread->local_storage_size  = size;
}

addr_t thread_get_local_storage(const thread_t *thread) {
    return thread->local_storage_addr;
}
