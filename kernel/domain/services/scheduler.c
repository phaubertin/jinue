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

#include <kernel/domain/entities/process.h>
#include <kernel/domain/services/panic.h>
#include <kernel/domain/services/scheduler.h>
#include <kernel/machine/spinlock.h>
#include <kernel/machine/thread.h>
#include <kernel/utils/list.h>

/** ready threads queue with lock */
static struct {
    list_t          queue;
    spinlock_t      lock;
} ready_queue = {
    .queue  = STATIC_LIST,
    .lock   = SPINLOCK_STATIC
};

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
thread_t *reschedule(bool current_can_run) {
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
 * Add a thread to the ready queue (without locking)
 * 
 * This funtion contains the business logic for thread_ready() without the
 * locking. Some functions beside thread_ready() that need to block and then
 * unlock call it, hence why it is a separate function.
 * 
 * @param thread the thread
 *
 */
static void thread_ready_locked(thread_t *thread) {
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
void thread_ready(thread_t *thread) {
    spin_lock(&ready_queue.lock);

    thread_ready_locked(thread);

    spin_unlock(&ready_queue.lock);
}

/**
 * Yield the current thread
 * 
 * The current thread is added at the tail of the ready queue. It continues
 * running if no other thread is ready to run.
 */
void thread_yield_current(void) {
    thread_t *current   = get_current_thread();
    thread_t *to        = reschedule(true);

    if(to == current) {
        return;
    }

    to->state = THREAD_STATE_RUNNING;

    if(current->process != to->process) {
        process_switch_to(to->process);
    }

    spin_lock(&ready_queue.lock);

    thread_ready_locked(current);

    machine_switch_thread_and_unlock(current, to, &ready_queue.lock);
}

/**
 * Switch to another thread
 * 
 * The current thread remains ready to run and is added to the ready queue.
 * 
 * @param to thread to switch to
 *
 */
void thread_switch_to(thread_t *to) {
    thread_t *current   = get_current_thread();

    to->state           = THREAD_STATE_RUNNING;

    if(current->process != to->process) {
        process_switch_to(to->process);
    }

    spin_lock(&ready_queue.lock);

    thread_ready_locked(current);

    machine_switch_thread_and_unlock(current, to, &ready_queue.lock);
}

/**
 * Switch to another thread and block the current thread
 * 
 * @param to thread to switch to
 *
 */
void thread_switch_to_and_block(thread_t *to) {
    thread_t *current   = get_current_thread();
    current->state      = THREAD_STATE_BLOCKED;
    to->state           = THREAD_STATE_RUNNING;

    if(current->process != to->process) {
        process_switch_to(to->process);
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
void thread_block_current_and_unlock(spinlock_t *lock) {
    thread_t *current   = get_current_thread();
    current->state      = THREAD_STATE_BLOCKED;

    thread_t *to        = reschedule(false);
    to->state           = THREAD_STATE_RUNNING;

    if(current->process != to->process) {
        process_switch_to(to->process);
    }

    machine_switch_thread_and_unlock(current, to, lock);
}
