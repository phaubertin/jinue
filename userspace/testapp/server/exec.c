/*
 * Copyright (C) 2026 Philippe Aubertin.
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

#include <jinue/jinue.h>
#include <jinue/loader.h>
#include <jinue/utils.h>
#include <srv/system.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "elf.h"
#include "exec.h"
#include "types.h"

static int initialize_descriptors(const process_t *process, const thread_t *thread, int endpoint) {
    int status = jinue_mint(
        process->fd,
        process->fd,
        SYS_DESC_SELF_PROCESS,
        JINUE_PERM_CREATE_THREAD | JINUE_PERM_MAP | JINUE_PERM_OPEN,
        0,
        &errno
    );

    if (status != 0) {
        jinue_error("error: could not create self process descriptor: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    status = jinue_mint(
        thread->fd,
        process->fd,
        SYS_DESC_MAIN_THREAD,
        JINUE_PERM_START | JINUE_PERM_AWAIT,
        0,
        &errno
    );

    if (status != 0) {
        jinue_error("error: could not create descriptor for initial thread: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    status = jinue_mint(
        endpoint,
        process->fd,
        SYS_DESC_ENDPOINT,
        JINUE_PERM_SEND,
        0,
        &errno
    );

    if (status != 0) {
        jinue_error("error: could not create descriptor for endpoint: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int exec(
    const process_t *process,
    const thread_t  *thread,
    const file_t    *exec_file,
    int              argc,
    char            *argv[],
    int              endpoint
) {
    thread_params_t thread_params;

    int status = load_elf(&thread_params, process, exec_file, argc, argv);

    if(status != EXIT_SUCCESS) {
        return status;
    }

    status = initialize_descriptors(process, thread, endpoint);

    if(status != EXIT_SUCCESS) {
        return status;
    }

    /* All signals are unblocked initially. */
    jinue_sigset_t sigset;
    jinue_sigemptyset(&sigset);

    status = jinue_start_thread(
        thread->fd,
        thread_params.entry,
        thread_params.stack_addr,
        &sigset,
        &errno
    );

    if(status != 0) {
        jinue_error("error: could not start thread: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
