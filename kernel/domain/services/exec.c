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

/**
 * Set up a predefined descriptor for the user space loader
 * 
 * @param process user space loader process
 * @param fd descriptor number
 * @param object object the descriptor will reference
 */
static void set_descriptor(process_t *process, int fd, object_header_t *object) {
    int status = descriptor_reserve_unused(process, fd);

    if(status < 0) {
        panic("Could not set up predefined descriptor for user space loader");
    }

    descriptor_t desc;
    desc.object = object;
    desc.flags  = object->type->all_permissions;
    desc.cookie = 0;

    descriptor_open(process, fd, &desc);
}

/**
 * Initialize the predefined descriptors for the user space loader
 * 
 * @param process user space loader process
 * @param thread initial thread
 */
static void initialize_descriptors(process_t *process, thread_t *thread) {
    set_descriptor(process, JINUE_DESC_SELF_PROCESS, process_object(process));
    set_descriptor(process, JINUE_DESC_MAIN_THREAD, thread_object(thread));
}

/**
 * Load an executable file into a new process and prepare the initial thread
 * 
 * This function is intended to load the user space loader binary, and any
 * other program will be loaded from user space. The executable file must be
 * a static binary.
 * 
 * This function sets up the loadable segments into the process address space
 * and prepares the initial thread with the proper entry point and stack
 * address. In addition, it also sets up two predefined descriptors: one that
 * refers to the process and another one to the thread. These descriptors have
 * the same purpose and descriptor numbers as two of the descriptors set up for
 * the initial process by the user space loader (see doc/init-process.md).
 * 
 * @param process user space loader process in which to load the executable
 * @param thread initial thread
 * @param exec_file executable binary file
 * @param argv0 program name (argv[0])
 * @param cmdline full kernel command line, used for arguments and environment
 */
void exec(
        process_t           *process,
        thread_t            *thread,
        const exec_file_t   *exec_file,
        const char          *argv0,
        const char          *cmdline) {
    
    thread_params_t thread_params;
    machine_load_exec(&thread_params, process, exec_file, argv0, cmdline);

    thread_prepare(thread, &thread_params);

    initialize_descriptors(process, thread);
}
