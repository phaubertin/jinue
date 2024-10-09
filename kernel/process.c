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
#include <kernel/i686/cpu_data.h>
#include <kernel/i686/thread.h>
#include <kernel/i686/vm.h>
#include <kernel/descriptor.h>
#include <kernel/panic.h>
#include <kernel/process.h>
#include <kernel/object.h>
#include <kernel/slab.h>
#include <stddef.h>
#include <string.h>

static void process_ctor(void *buffer, size_t ignore);

/** runtime type definition for a process */
static const object_type_t object_type = {
    .all_permissions    = 0,
    .name               = "process",
    .size               = sizeof(process_t),
    .cache_ctor         = process_ctor,
    .cache_dtor         = NULL
};

const object_type_t *object_type_process = &object_type;

/** slab cache used for allocating process objects */
static slab_cache_t process_cache;

static void process_ctor(void *buffer, size_t ignore) {
    process_t *process = buffer;

    object_header_init(&process->header, object_type_process);
}

void process_boot_init(void) {
    object_cache_init(&process_cache, object_type_process);
}

static void process_init(process_t *process) {
    memset(&process->descriptors, 0, sizeof(process->descriptors));
}

process_t *process_create(void) {
    process_t *process = slab_cache_alloc(&process_cache);

    if(process != NULL) {
        addr_space_t *addr_space = vm_create_addr_space(&process->addr_space);

        /* The address space object is located inside the process object but the
         * call to vm_create_addr_space() above can still fail if we cannot
         * allocate the paging translation tables. */
        if(addr_space == NULL) {
            slab_cache_free(process);
            return NULL;
        }

        process_init(process);
    }

    return process;
}

void process_destroy(process_t *process) {
    /* TODO finalize descriptors */
    vm_destroy_addr_space(&process->addr_space);
    slab_cache_free(process);
}

int process_create_syscall(int fd) {
    object_ref_t *ref;
    thread_t *thread = get_current_thread();
    int status = dereference_unused_descriptor(&ref, thread->process, fd);

    if(status < 0) {
        return status;
    }

    process_t *process = process_create();

    if(process == NULL) {
        return -JINUE_EAGAIN;
    }

    object_addref(&process->header);

    ref->object = &process->header;
    ref->flags  = OBJECT_REF_FLAG_IN_USE | OBJECT_REF_FLAG_OWNER;
    ref->cookie = 0;

    return 0;
}

void process_switch_to(process_t *process) {
    vm_switch_addr_space(
            &process->addr_space,
            get_cpu_local_data());
}
