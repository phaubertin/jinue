/*
 * Copyright (C) 2019-2024 Philippe Aubertin.
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

#include <kernel/domain/alloc/page_alloc.h>
#include <kernel/domain/entities/descriptor.h>
#include <kernel/domain/entities/object.h>
#include <kernel/domain/entities/process.h>
#include <kernel/domain/entities/thread.h>
#include <kernel/domain/services/panic.h>
#include <kernel/machine/spinlock.h>
#include <kernel/machine/thread.h>
#include <kernel/utils/list.h>

static void free_thread(object_header_t *object);

/** runtime type definition for a thread */
static const object_type_t object_type = {
    .all_permissions    = JINUE_PERM_START | JINUE_PERM_AWAIT,
    .name               = "thread",
    .size               = sizeof(thread_t),
    .open               = NULL,
    .close              = NULL,
    .destroy            = NULL,
    .free               = free_thread,
    .cache_ctor         = NULL,
    .cache_dtor         = NULL
};

const object_type_t *object_type_thread = &object_type;

static struct {
    jinue_list_t    queue;
    spinlock_t      lock;
} ready_queue = {
    .queue  = JINUE_LIST_STATIC,
    .lock   = SPINLOCK_STATIC
};

thread_t *construct_thread(process_t *process) {
    thread_t *thread = machine_alloc_thread();

    if(thread == NULL) {
        return NULL;
    }

    init_object_header(&thread->header, object_type_thread);

    jinue_node_init(&thread->thread_list);

    thread->state               = THREAD_STATE_ZOMBIE;
    thread->process             = process;
    /* Arbitrary non-NULL value to signify the thread hasn't run yet and
     * shouldn't be awaited. This will fall in the condition in await_thread()
     * that detects an attempt to await a thread that has already been awaited,
     * so await_thread() will fail with JINUE_ESRCH. */
    thread->awaiter             = thread;
    thread->local_storage_addr  = NULL;
    thread->local_storage_size  = 0;
 
    return thread;
}

static void free_thread(object_header_t *object) {
    thread_t *thread = (thread_t *)object;
    machine_free_thread(thread);
}

void prepare_thread(thread_t *thread, const thread_params_t *params) {
    thread->sender  = NULL;
    thread->awaiter = NULL;
    machine_prepare_thread(thread, params);
}

void ready_thread(thread_t *thread) {
    thread->state = THREAD_STATE_READY;

    spin_lock(&ready_queue.lock);

    /* add thread to the tail of the ready list to give other threads a chance to run */
    jinue_list_enqueue(&ready_queue.queue, &thread->thread_list);

    spin_unlock(&ready_queue.lock);
}

thread_t *dequeue_ready_thread(void) {
    spin_lock(&ready_queue.lock);

    thread_t *thread = jinue_node_entry(
            jinue_list_dequeue(&ready_queue.queue),
            thread_t,
            thread_list
    );

    spin_unlock(&ready_queue.lock);

    return thread;
}

static thread_t *reschedule(bool current_can_run) {
    thread_t *to_thread = dequeue_ready_thread();
    
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

static void switch_thread(thread_t *from, thread_t *to, bool destroy_from) {
    if(to == from) {
        return;
    }

    if(from == NULL || from->process != to->process || destroy_from) {
        switch_to_process(to->process);
    }

    to->state = THREAD_STATE_RUNNING;

    if(destroy_from) {
        machine_switch_and_unref_thread(from, to);
    } else {
        machine_switch_thread(from, to);
    }
}

void switch_to_thread(thread_t *thread, bool blocked) {
    thread_t *current = get_current_thread();

    if (blocked) {
        current->state = THREAD_STATE_BLOCKED;
    } else {
        ready_thread(current);
    }

    switch_thread(
            current,
            thread,
            false               /* don't destroy current thread */
    );
}

void start_first_thread(thread_t *thread) {
    thread_is_starting(thread);

    switch_thread(
            NULL,
            thread,
            false               /* don't destroy current thread */
    );
}

void yield_current_thread(void) {
    switch_to_thread(
            reschedule(true),   /* current thread can run */
            false               /* don't block current thread */
    );
}

void block_current_thread(void) {
    switch_to_thread(
            reschedule(false),  /* current thread cannot run */
            true                /* do block current thread */
    );
}

void thread_is_starting(thread_t *thread) {
    add_running_thread_to_process(thread->process);
    
    /* Add a reference on the thread while it is running so it is allowed to
     * run to completion even if all descriptors that reference it get closed. */
    add_ref_to_object(&thread->header);
}

void current_thread_is_exiting(void) {
    thread_t *thread = get_current_thread();

    thread->state = THREAD_STATE_ZOMBIE;

    remove_running_thread_from_process(thread->process);

    /* switch_thread() takes care of safely decrementing the reference count on
     * the thread after having switched to another one. We cannot just do it
     * here because that will possibly free the current thread, which we don't
     * want to do while it is still running. */
    switch_thread(
            thread,
            reschedule(false),  /* current thread cannot run */
            true                /* decrement reference count */
    );
}

void set_thread_local_storage(thread_t *thread, addr_t addr, size_t size) {
    thread->local_storage_addr = addr;
    thread->local_storage_size = size;
}
