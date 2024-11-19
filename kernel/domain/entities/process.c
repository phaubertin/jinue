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

#include <jinue/shared/asm/errno.h>
#include <kernel/domain/alloc/slab.h>
#include <kernel/domain/entities/descriptor.h>
#include <kernel/domain/entities/process.h>
#include <kernel/domain/entities/object.h>
#include <kernel/machine/atomic.h>
#include <kernel/machine/pmap.h>
#include <kernel/machine/process.h>
#include <kernel/machine/thread.h>
#include <stddef.h>
#include <string.h>

static void cache_process_ctor(void *buffer, size_t ignore);

static void destroy_process(object_header_t *object);

static void free_process(object_header_t *object);

static const object_type_t object_type = {
    .all_permissions    =
              JINUE_PERM_CREATE_THREAD
            | JINUE_PERM_MAP
            | JINUE_PERM_OPEN,
    .name               = "process",
    .size               = sizeof(process_t),
    .open               = NULL,
    .close              = NULL,
    .destroy            = destroy_process,
    .free               = free_process,
    .cache_ctor         = cache_process_ctor,
    .cache_dtor         = NULL
};

/** runtime type definition for a process */
const object_type_t *object_type_process = &object_type;

/** slab cache used for allocating process objects */
static slab_cache_t process_cache;

/**
 * Constructor for process object in slab cache
 * 
 * This constructor is called when the slab cache is grown. It should only
 * initialize state that persists when the object is freed and then reused,
 * such as the object type.
 * 
 * See construct_process() for the run time constructor.
 * 
 * @param buffer the process to construct
 * @param ignore size of object - ignored
 */
static void cache_process_ctor(void *buffer, size_t ignore) {
    process_t *process = buffer;
    init_object_header(&process->header, object_type_process);
}

/**
 * Process slab cache initialization
 */
void initialize_process_cache(void) {
    init_object_cache(&process_cache, object_type_process);
}

/**
 * Initialize the descriptors of a process being constructed
 * 
 * @param process the process being constructed
 */
static void initialize_descriptors(process_t *process) {
    for(int idx = 0; idx < JINUE_DESC_NUM; ++idx) {
        descriptor_clear(&process->descriptors[idx]);
    }
}

/**
 * Process constructor
 * 
 * @return process if successful, NULL if out of memory
 */
process_t *construct_process(void) {
    process_t *process = slab_cache_alloc(&process_cache);

    if(process != NULL) {
        initialize_descriptors(process);

        if(!machine_init_process(process)) {
            slab_cache_free(process);
            return NULL;
        }
    }

    return process;
}

/**
 * Close all descriptors of a process being destroyed
 * 
 * @param process the process being destroyed
 */
static void close_descriptors(process_t *process) {
    for(int idx = 0; idx < JINUE_DESC_NUM; ++idx) {
        descriptor_t *desc = &process->descriptors[idx];

        if(descriptor_is_open(desc)) {
            close_object(desc->object, desc);
        }
    }
}

/**
 * Destroy a process
 * 
 * This is the "destroy" op of the object type, hence why the type of the
 * argument.
 * 
 * @param object process object
 */
static void destroy_process(object_header_t *object) {
    process_t *process = (process_t *)object;
    /* TODO destroy remaining threads */
    close_descriptors(process);
    machine_finalize_process(process);
}

/**
 * Free a process
 * 
 * This is the "free" op of the object type, hence why the type of the
 * argument. This function is called automatically once the process no
 * longer has any references.
 * 
 * @param object process object
 */
static void free_process(object_header_t *object) {
    slab_cache_free(object);
}

/**
 * Switch to specified process address space
 * 
 * @param process the process
 */
void switch_to_process(process_t *process) {
    machine_switch_to_process(process);
}

/**
 * Get process running on current CPU
 * 
 * @return running process
 */
process_t *get_current_process(void) {
    return get_current_thread()->process;
}

/**
 * Update process state to account for a new running thread
 * 
 * This function is called not when a new thread is created but when it
 * actually starts running.
 * 
 * @param process the process that gained a running thread
 */
void add_running_thread_to_process(process_t *process) {
    add_atomic(&process->running_threads_count, 1);
    add_ref_to_object(&process->header);
}

/**
 * Update process state to account for a running thread exiting
 * 
 * This function is called not when a thread is destroyed but when it exits.
 * When a thread has exited, the kernel thread object can be reused for
 * another application thread. However, when *all* of a process' threads have
 * exited, the process is destroyed, which this funtion takes care of.
 * 
 * @param process the process that lost a running thread
 */
void remove_running_thread_from_process(process_t *process) {
    int running_count = add_atomic(&process->running_threads_count, -1);
    
    /* Destroy the process when there are no more running threads. The
     * reference count alone is not enough because the process might have
     * descriptors that reference itself. */
    if(running_count < 1) {
        destroy_object(&process->header);
    }
    
    sub_ref_to_object(&process->header);
}
