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

#include <jinue/shared/list.h>
#include <hal/cpu_data.h>
#include <hal/thread.h>
#include <hal/vm.h>
#include <boot.h>
#include <object.h>
#include <process.h>
#include <page_alloc.h>
#include <panic.h>
#include <thread.h>


static jinue_list_t ready_list = JINUE_LIST_STATIC;

static void thread_init(thread_t *thread, process_t *process) {
    object_header_init(&thread->header, OBJECT_TYPE_THREAD);

    jinue_node_init(&thread->thread_list);

    thread->process     = process;
    thread->sender      = NULL;

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

void thread_switch(
        thread_t    *from_thread,
        thread_t    *to_thread,
        bool         blocked,
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

            /* Put the the thread we are switching away from (the current thread)
             * back into the ready list, unless it just blocked or it is being
             * destroyed. */
            if(! (do_destroy || blocked)) {
                thread_ready(from_thread);
            }
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

static thread_t *reschedule(thread_t *from_thread, bool from_can_run) {
    thread_t *to_thread = jinue_node_entry(
            jinue_list_dequeue(&ready_list),
            thread_t,
            thread_list );
    
    if(to_thread == NULL) {
        if(from_thread != NULL && from_can_run) {
            /* We just let the current thread run because there are no other
             * threads to run. */
            return from_thread;
        }
        else {
            /* Currently, scheduling is purely cooperative and only one CPU is
             * supported (so, there are no threads currently running on other
             * CPUs). What this means is that, once there are no more threads
             * running or ready to run, this situation will never change. */
            panic("No more thread to schedule");
        }
    }

    return to_thread;
}

static void thread_yield_from(
        thread_t    *from_thread,
        bool         blocked,
        bool         do_destroy) {

    bool from_can_run = !(blocked || do_destroy);
    
    thread_switch(
            from_thread,
            reschedule(from_thread, from_can_run),
            blocked,
            do_destroy);
}

void thread_start_first(void) {
    thread_yield_from(
            NULL,
            false,      /* don't block */
            false);     /* don't destroy */
}

void thread_yield(void) {
    thread_yield_from(
            get_current_thread(),
            false,      /* don't block */
            false);     /* don't destroy the thread */
}

void thread_block(void) {
    thread_yield_from(
            get_current_thread(),
            true,       /* do block */
            false);     /* don't destroy the thread */
}

void thread_exit(void) {
    thread_yield_from(
            get_current_thread(),
            false,      /* don't block */
            true);      /* do destroy the thread */
}
