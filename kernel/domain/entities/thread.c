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
#include <kernel/domain/alloc/page_alloc.h>
#include <kernel/domain/entities/descriptor.h>
#include <kernel/domain/entities/object.h>
#include <kernel/domain/entities/process.h>
#include <kernel/domain/entities/thread.h>
#include <kernel/domain/services/panic.h>
#include <kernel/machine/thread.h>
#include <kernel/utils/list.h>

/** runtime type definition for a thread */
static const object_type_t object_type = {
    .all_permissions    = 0,
    .name               = "thread",
    .size               = sizeof(thread_t),
    .open               = NULL,
    .close              = NULL,
    .destroy            = NULL,
    .free               = NULL,
    .cache_ctor         = NULL,
    .cache_dtor         = NULL
};

const object_type_t *object_type_thread = &object_type;

static jinue_list_t ready_list = JINUE_LIST_STATIC;

thread_t *construct_thread(process_t *process, const thread_params_t *params) {
    thread_t *thread = machine_alloc_thread();

    if(thread == NULL) {
        return NULL;
    }

    init_object_header(&thread->header, object_type_thread);

    jinue_node_init(&thread->thread_list);

    thread->process             = process;
    thread->sender              = NULL;
    thread->local_storage_addr  = NULL;
    thread->local_storage_size  = 0;

    machine_init_thread(thread, params);

    ready_thread(thread);
    
    return thread;
}

/* This function is called by assembly code. See thread_context_switch_stack(). */
void free_thread(thread_t *thread) {
    machine_free_thread(thread);
}

void ready_thread(thread_t *thread) {
    /* add thread to the tail of the ready list to give other threads a chance
     * to run */
    jinue_list_enqueue(&ready_list, &thread->thread_list);
}

static thread_t *reschedule(bool current_can_run) {
    thread_t *to_thread = jinue_node_entry(
            jinue_list_dequeue(&ready_list),
            thread_t,
            thread_list
    );
    
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

    if(from == NULL || from->process != to->process) {
        switch_to_process(to->process);
    }

    machine_switch_thread(from, to, destroy_from);
}

void switch_to_thread(thread_t *thread, bool blocked) {
    thread_t *current = get_current_thread();

    if (!blocked) {
        ready_thread(current);
    }

    switch_thread(
            current,
            thread,
            false               /* don't destroy current thread */
    );
}

void start_first_thread(void) {
    switch_thread(
            NULL,
            reschedule(false),
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

void exit_current_thread(void) {
    switch_thread(
            get_current_thread(),
            reschedule(false),  /* current thread cannot run */
            true                /* do destroy the thread */
    );
}

void set_thread_local_storage(thread_t *thread, addr_t addr, size_t size) {
    thread->local_storage_addr = addr;
    thread->local_storage_size = size;
}

addr_t get_thread_local_storage(const thread_t *thread) {
    return thread->local_storage_addr;
}
