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

#include <jinue/shared/asm/errno.h>
#include <kernel/domain/alloc/page_alloc.h>
#include <kernel/domain/entities/object.h>
#include <kernel/domain/entities/process.h>
#include <kernel/domain/entities/thread.h>
#include <kernel/domain/services/ipc.h>
#include <kernel/domain/services/scheduler.h>
#include <kernel/machine/spinlock.h>
#include <kernel/machine/thread.h>
#include <kernel/machine/tls.h>

static void free_op(object_header_t *object);

static const object_type_t object_type = {
    .all_permissions    = JINUE_PERM_START | JINUE_PERM_AWAIT,
    .name               = "thread",
    .size               = sizeof(thread_t),
    .open               = NULL,
    .close              = NULL,
    .destroy            = NULL,
    .free               = free_op,
    .cache_ctor         = NULL,
    .cache_dtor         = NULL
};

/** runtime type definition for a thread */
const object_type_t *object_type_thread = &object_type;

/**
 * Thread constructor
 * 
 * The in-kernel thread implementation separates thread creation and thread
 * startup, which allows an application to keep kernel thread objects around in
 * a thread pool and reuse them as application threads exit and new ones start.
 * This function constructs a thread but does not start it. thread_run() (or
 * thread_run_first()) does that on an already constructed thread that has
 * been prepared for a first or new run with thread_prepare().
 * 
 * @param process process in which to create the new thread
 * @return thread on success, NULL on memory allocation error
 *
 */
thread_t *thread_new(process_t *process) {
    thread_t *thread = machine_alloc_thread();

    if(thread == NULL) {
        return NULL;
    }

    object_init_header(&thread->header, object_type_thread);

    init_spinlock(&thread->await_lock);

    thread->state               = THREAD_STATE_CREATED;
    thread->process             = process;
    thread->awaiter             = NULL;
    thread->local_storage_addr  = NULL;
    thread->local_storage_size  = 0;
 
    return thread;
}

/**
 * Free a thread
 * 
 * This function implements the "free" operation of the runtime type
 * definition.
 * 
 * @param object object header of thread object
 *
 */
static void free_op(object_header_t *object) {
    thread_t *thread = (thread_t *)object;
    machine_free_thread(thread);
}

/**
 * Prepare a thread to be run
 * 
 * Initializes the thread state. This initialization includes preparing the
 * context on the stack with the specified initial stack pointer and entry
 * point.
 * 
 * This function is separate from the thread constructor to allow a thread
 * to be reused by the application. (See thread_new())
 * 
 * Once a thread has been prepared by calling this function, it can be run
 * by calling thread_run() (or thread_run_first()).
 * 
 * @param thread the thread
 * @param params initialization parameters
 *
 */
void thread_prepare(thread_t *thread, const thread_params_t *params) {
    thread->sender  = NULL;
    
    spin_lock(&thread->await_lock);
    
    thread->awaiter = NULL;
    thread->state   = THREAD_STATE_STARTING;
    thread->cpu_credits = 0;

    spin_unlock(&thread->await_lock);    
    
    machine_prepare_thread(thread, params);
}

/**
 * Common logic for a starting thread
 * 
 * @param thread the thread that is starting
 *
 */
static void thread_is_starting(thread_t *thread) {
    process_add_running_thread(thread->process);
    
    /* Add a reference on the thread while it is running so it is allowed to
     * run to completion even if all descriptors that reference it get closed. */
    object_add_ref(&thread->header);

    thread->state = THREAD_STATE_RUNNING;
}

/**
 * Run the first thread
 * 
 * This function must not call get_current_thread() because that function will
 * not find a thread_t structure where it expects to find it relative to the
 * stack pointer. It must also actually switch to the thread instead of just
 * adding it to the ready queue for the thread to actually run.
 * 
 * @param thread the thread to run
 *
 */
void thread_run_first(thread_t *thread) {
    process_switch_to(thread->process);

    thread_is_starting(thread);

    machine_switch_thread(NULL, thread);
}

/**
 * Run a thread
 * 
 * Before this function is called, the thread must have been prepared with
 * thread_prepare().
 * 
 * @param thread the thread to run
 *
 */
void thread_run(thread_t *thread) {
    thread_is_starting(thread);

    ready_thread(thread);
}

/**
 * Terminate the current thread
 * 
 * The thread is destroyed and freed only if there are no more references to it
 * (i.e. no descriptors referencing it). Otherwise, it remains available to be
 * reused by calling thread_prepare() and then thread_run() again.
 */
void thread_terminate_current(void) {
    thread_t *current = get_current_thread();

    spin_lock(&current->await_lock);

    /* This state transition must be done under lock to avoid a race condition
     * with await_thread(). */
    current->state = THREAD_STATE_ZOMBIE;

    if(current->awaiter != NULL) {
        ready_thread(current->awaiter);
    }

    spin_unlock(&current->await_lock);

    if(current->sender != NULL) {
        abort_message(current->sender);
        current->sender = NULL;
    }

    switch_from_exiting_thread();
}

/**
 * Block until thread terminates
 * 
 * @param thread awaited thread
 * @return zero on success, negated error code on failure
 *
 */
int thread_await(thread_t *thread) {
    thread_t *current = get_current_thread();

    if(thread == current) {
        return -JINUE_EDEADLK;
    }

    spin_lock(&thread->await_lock);

    if(thread->state == THREAD_STATE_CREATED || thread->awaiter != NULL) {
        spin_unlock(&thread->await_lock);
        return -JINUE_ESRCH;
    }

    thread->awaiter = current;

    if(thread->state == THREAD_STATE_ZOMBIE) {
        spin_unlock(&thread->await_lock);
    } else {
        block_current_thread_and_unlock(&thread->await_lock);
    }

    return 0;
}

/**
 * Set the address and size of the Thread-Local Storage (TLS)
 * 
 * @param thread the thread
 * @param addr address of thread-local storage
 * @param size size of thread-local storage
 *
 */
void thread_set_local_storage(thread_t *thread, addr_t addr, size_t size) {
    thread->local_storage_addr = addr;
    thread->local_storage_size = size;

    if(thread == get_current_thread()) {
        machine_set_thread_local_storage(thread);
    }
}
