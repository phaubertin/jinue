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
#include <internals.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../utils.h"
#include "handlers/handlers.h"
#include "debug.h"
#include "exec.h"
#include "ramdisk.h"
#include "types.h"
#include "utils.h"

#define RECV_BUFFER_SIZE 512

static int run_server(int fd, const message_context_t *message_context) {
    while(true) {
        unsigned char buffer[RECV_BUFFER_SIZE];

        jinue_buffer_t recv_buffer;
        recv_buffer.addr = &buffer;
        recv_buffer.size = sizeof(buffer);

        jinue_message_t message;
        message.recv_buffers        = &recv_buffer;
        message.recv_buffers_length = 1;

        intptr_t len = jinue_receive(fd, &message, &errno);

        if(len < 0) {
            jinue_error("jinue_receive() failed: %s", strerror(errno));
            return EXIT_FAILURE;
        }

        switch(message.recv_function) {
            case SYS_MSG_EXIT:
                /* At this point, the remote process is done. No need to send a reply. */
                return EXIT_SUCCESS;
            case SYS_MSG_MAP_ANON:
                handle_map_anon(message_context, buffer, len);
                break;
            default:
                reply_error(ENOSYS);
        }
    }
}

static int do_main(int argc, char *argv[]) {
    /* Say hello. */
    jinue_info("Jinue test app server (%s) started.", argv[0]);

    ramdisk_t ramdisk;

    int status = get_ramdisk(&ramdisk);

    if(status != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    dump_cmdline_arguments(argc, argv);
    dump_environ();
    dump_auxvec();
    dump_syscall_implementation();
    dump_address_map();
    dump_loader_memory_info();
    dump_ramdisk(&ramdisk);

    jinue_info("Blocking until loader exits...");

    status = jinue_exit_loader();

    if(status < 0) {
        return EXIT_FAILURE;
    }

    jinue_info("Loader has exited.");

    jinue_info("Creating server endpoint.");

    int endpoint = libc_allocate_descriptor();

    if(endpoint < 0) {
        jinue_error("error: libc_allocate_descriptor() failed: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    status = jinue_create_endpoint(endpoint, &errno);

    if(status < 0) {
        jinue_error("error: could not create IPC endpoint: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    jinue_info("Creating client process.");

    message_context_t message_context;
    process_t *process = &message_context.process;
    process->fd = libc_allocate_descriptor();

    if(process->fd < 0) {
        jinue_error("error: libc_allocate_descriptor() failed: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    status = jinue_create_process(process->fd, &errno);

    if(status < 0) {
        jinue_error("error: could not create process: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    jinue_info("Creating client process main thread.");

    thread_t thread;
    thread.fd = libc_allocate_descriptor();

    if(thread.fd < 0) {
        jinue_error("error: libc_allocate_descriptor() failed: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    status = jinue_create_thread(thread.fd, process->fd, &errno);

    if(status < 0) {
        jinue_error("error: could not create thread: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    jinue_info("Running client process.");

    file_t exec_file;

    status = open_ramdisk_file(&exec_file, &ramdisk, "/sbin/testapp");

    if(status != EXIT_SUCCESS) {
        return status;
    }

    jinue_info("---");

    status = exec(process, &thread, &exec_file, argc, argv, endpoint);

    if(status != EXIT_SUCCESS) {
        jinue_error("error: could not load client ELF file: %s", strerror(errno));
        return status;
    }

    return run_server(endpoint, &message_context);
}

int main(int argc, char *argv[]) {
    int status = do_main(argc, argv);

    if(bool_getenv("DEBUG_DO_REBOOT")) {
        jinue_info("Rebooting.");
        jinue_reboot();
    }

    if(status != EXIT_SUCCESS) {
        return status;
    }

    while (1) {
        jinue_yield_thread();
    }
    
    return EXIT_SUCCESS;
}
