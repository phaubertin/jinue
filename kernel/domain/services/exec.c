/*
 * Copyright (C) 2024 Philippe Aubertin.
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

#include <jinue/shared/asm/descriptors.h>
#include <kernel/domain/entities/descriptor.h>
#include <kernel/domain/entities/object.h>
#include <kernel/domain/entities/process.h>
#include <kernel/domain/entities/thread.h>
#include <kernel/domain/services/exec.h>
#include <kernel/domain/services/panic.h>
#include <kernel/machine/exec.h>

static void set_descriptor(process_t *process, int fd, object_header_t *object) {
    (void)reserve_unused_descriptor(process, fd);

    descriptor_t desc;
    desc.object = object;
    desc.flags  = object->type->all_permissions;
    desc.cookie = 0;

    open_descriptor(process, fd, &desc);
}

static void initialize_descriptors(process_t *process, thread_t *thread) {
    set_descriptor(process, JINUE_DESC_SELF_PROCESS, process_object(process));
    set_descriptor(process, JINUE_DESC_MAIN_THREAD, thread_object(thread));
}

void exec(
        process_t           *process,
        thread_t            *thread,
        const exec_file_t   *exec_file,
        const char          *argv0,
        const char          *cmdline) {
    
    thread_params_t thread_params;
    machine_load_exec(&thread_params, process, exec_file, argv0, cmdline);

    prepare_thread(thread, &thread_params);

    initialize_descriptors(process, thread);
}
