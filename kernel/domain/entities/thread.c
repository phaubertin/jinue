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
#include <kernel/domain/services/ipc.h>
#include <kernel/domain/services/panic.h>
#include <kernel/machine/spinlock.h>
#include <kernel/machine/thread.h>
#include <kernel/machine/tls.h>
#include <kernel/utils/list.h>

static void free_thread(object_header_t *object);

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

/** runtime type definition for a thread */
const object_type_t *object_type_thread = &object_type;

/** ready threads queue with lock */
static struct {
    list_t          queue;
    spinlock_t      lock;
} ready_queue = {
    .queue  = STATIC_LIST,
    .lock   = SPINLOCK_STATIC
};

/**
 * Thread constructor
 * 
 * The in-kernel thread implementation separates thread creation and thread
 * startup, which allows an application to keep kernel thread objects around in
 * a thread pool and reuse them as application threads exit and new ones start.
 * This function constructs a thread but does not start it. run_thread() (or
 * run_first_thread()) does that on an already constructed thread that has
 * been prepared for a first or new run with prepare_thread().
 * 
 * @param process process in which to create the new thread
 * @return thread on success, NULL on memory allocation error
 *
 */
thread_t *construct_thread(process_t *process) {
    thread_t *thread = machine_alloc_thread();

    if(thread == NULL) {
        return NULL;
    }

    init_object_header(&thread->header, object_type_thread);

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
static void free_thread(object_header_t *object) {
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
 * to be reused by the application. (See construct_thread())
 * 
 * Once a thread has been prepared by calling this function, it can be run
 * by calling run_thread() (or run_first_thread()).
 * 
 * @param thread the thread
 * @param params initialization parameters
 *
 */
void prepare_thread(thread_t *thread, const thread_params_t *params) {
    thread->sender  = NULL;
    
    spin_lock(&thread->await_lock);
    
    thread->awaiter = NULL;
    thread->state   = THREAD_STATE_STARTING;

    spin_unlock(&thread->await_lock);    
    
    machine_prepare_thread(thread, params);
}

/**
 * Add a thread to the ready queue (without locking)
 * 
 * This funtion contains the business logic for ready_thread() without the
 * locking. Some functions beside ready_thread() that need to block and then
 * unlock call it, hence why it is a separate function.
 * 
 * @param thread the thread
 *
 */
static void ready_thread_locked(thread_t *thread) {
    thread->state = THREAD_STATE_READY;

    /* add thread to the tail of the ready list to give other threads a chance to run */
    list_enqueue(&ready_queue.queue, &thread->thread_list);
}

/**
 * Add a thread to the ready queue
 * 
 * @param thread the thread
 *
 */
void ready_thread(thread_t *thread) {
    spin_lock(&ready_queue.lock);

    ready_thread_locked(thread);

    spin_unlock(&ready_queue.lock);
}

/**
 * Common logic for a starting thread
 * 
 * @param thread the thread that is starting
 *
 */
static void thread_is_starting(thread_t *thread) {
    add_running_thread_to_process(thread->process);
    
    /* Add a reference on the thread while it is running so it is allowed to
     * run to completion even if all descriptors that reference it get closed. */
    add_ref_to_object(&thread->header);

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
void run_first_thread(thread_t *thread) {
    switch_to_process(thread->process);

    thread_is_starting(thread);

    machine_switch_thread(NULL, thread);
}

/**
 * Run a thread
 * 
 * Before this function is called, the thread must have been prepared with
 * prepare_thread().
 * 
 * @param thread the thread to run
 *
 */
void run_thread(thread_t *thread) {
    thread_is_starting(thread);

    ready_thread(thread);
}

/**
 * Get the thread at the head of the ready queue
 * 
 * @return thread ready to run, NULL if there are none
 *
 */
static thread_t *dequeue_ready_thread(void) {
    spin_lock(&ready_queue.lock);

    thread_t *thread = list_dequeue(&ready_queue.queue, thread_t, thread_list);

    spin_unlock(&ready_queue.lock);

    return thread;
}

/**
 * Get the next thread to run
 * 
 * @param current_can_run whether the current thread can continue running
 * @return thread ready to run
 *
 */
static thread_t *reschedule(bool current_can_run) {
    thread_t *to = dequeue_ready_thread();
    
    if(to == NULL) {
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

    return to;
}

/**
 * Terminate the current thread
 * 
 * The thread is destroyed and freed only if there are no more references to it
 * (i.e. no descriptors referencing it). Otherwise, it remains available to be
 * reused by calling prepare_thread() and then run_thread() again.
 */
void terminate_current_thread(void) {
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

    thread_t *to    = reschedule(false);
    to->state       = THREAD_STATE_RUNNING;
    
    if(current->process != to->process) {
        switch_to_process(to->process);
    }

    /* This must be done after switching process since it will destroy the process
     * if the current thread is the last one. We don't want to destroy the address
     * space we are still running in... */
    remove_running_thread_from_process(current->process);

    /* This function takes care of safely decrementing the reference count on
     * the thread after having switched to the other one. We cannot just do it
     * here because that will possibly free the current thread, which we don't
     * want to do while it is still running. */
    machine_switch_and_unref_thread(current, to);
}

/**
 * Switch to another thread
 * 
 * The current thread remains ready to run and is added to the ready queue.
 * 
 * @param to thread to switch to
 *
 */
void switch_to(thread_t *to) {
    thread_t *current   = get_current_thread();

    to->state           = THREAD_STATE_RUNNING;

    if(current->process != to->process) {
        switch_to_process(to->process);
    }

    spin_lock(&ready_queue.lock);

    ready_thread_locked(current);

    machine_switch_thread_and_unlock(current, to, &ready_queue.lock);
}

/**
 * Switch to another thread and block the current thread
 * 
 * @param to thread to switch to
 *
 */
void switch_to_and_block(thread_t *to) {
    thread_t *current   = get_current_thread();
    current->state      = THREAD_STATE_BLOCKED;
    to->state           = THREAD_STATE_RUNNING;

    if(current->process != to->process) {
        switch_to_process(to->process);
    }

    machine_switch_thread(current, to);
}

/**
 * Block the current thread and then unlock a lock
 * 
 * The lock is unlocked *after* the switch to another thread. This function
 * eliminates race conditions when enqueuing the current thread to a queue,
 * setting it as the awaiter of another thread, etc. and then blocking, if
 * the following sequence is followed:
 * 
 *  1. Take the lock (e.g. the lock protecting a queue).
 *  2. Add the thread (e.g. to the queue).
 *  3. Call this function to block the thread and release the lock atomically.
 * 
 * @param lock the lock to unlock after switching thread
 *
 */
void block_and_unlock(spinlock_t *lock) {
    thread_t *current   = get_current_thread();
    current->state      = THREAD_STATE_BLOCKED;

    thread_t *to        = reschedule(false);
    to->state           = THREAD_STATE_RUNNING;

    if(current->process != to->process) {
        switch_to_process(to->process);
    }

    machine_switch_thread_and_unlock(current, to, lock);
}

/**
 * Yield the current thread
 * 
 * The current thread is added at the tail of the ready queue. It continues
 * running if no other thread is ready to run.
 */
void yield_current_thread(void) {
    thread_t *current   = get_current_thread();
    thread_t *to        = reschedule(true);

    if(to == current) {
        return;
    }

    to->state = THREAD_STATE_RUNNING;

    if(current->process != to->process) {
        switch_to_process(to->process);
    }

    spin_lock(&ready_queue.lock);

    ready_thread_locked(current);

    machine_switch_thread_and_unlock(current, to, &ready_queue.lock);
}

/**
 * Set the address and size of the Thread-Local Storage (TLS)
 * 
 * @param thread the thread
 * @param addr address of thread-local storage
 * @param size size of thread-local storage
 *
 */
void set_thread_local_storage(thread_t *thread, addr_t addr, size_t size) {
    thread->local_storage_addr = addr;
    thread->local_storage_size = size;

    if(thread == get_current_thread()) {
        machine_set_thread_local_storage(thread);
    }
}
