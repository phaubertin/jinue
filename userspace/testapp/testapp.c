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

#include <jinue/jinue.h>
#include <jinue/loader.h>
#include <jinue/utils.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "tests/ipc.h"
#include "debug.h"
#include "utils.h"

#define MAP_BUFFER_SIZE 16384

void dump_user_memory(void) {
    char call_buffer[MAP_BUFFER_SIZE];

    /* get free memory blocks from microkernel */
    int status = jinue_get_user_memory(
        (jinue_mem_map_t *)&call_buffer,
        sizeof(call_buffer),
        &errno
    );

    if(status != 0) {
        jinue_error("error: could not get memory map from kernel: %s", strerror(errno));
        return;
    }

    dump_phys_memory_map((jinue_mem_map_t *)&call_buffer);
}

void dump_loader_memory(void) {
    char buffer[MAP_BUFFER_SIZE];

    jinue_buffer_t reply_buffer;
    reply_buffer.addr = buffer;
    reply_buffer.size = sizeof(buffer);

    jinue_message_t message;
    message.send_buffers        = NULL;
    message.send_buffers_length = 0;
    message.recv_buffers        = &reply_buffer;
    message.recv_buffers_length = 1;

    jinue_info("Sending message with unsupported message number.");

    uintptr_t errcode;
    int status = jinue_send(JINUE_DESC_LOADER_ENDPOINT, 16777, &message, &errno, &errcode);

    if(status >= 0) {
        jinue_error("error: jinue_send() unexpectedly succeeded");
        return;
    }

    if(errno != JINUE_EPROTO) {
        jinue_error("error: jinue_send() failed: %s.", strerror(errno));
        return;
    }

    jinue_info("expected: jinue_send() set errno to: %s.", strerror(errno));

    if(errcode != JINUE_ENOSYS) {
        jinue_error("error: loader set error code: %#" PRIxPTR ".", errcode);
        return;
    }

    jinue_info("expected: loader set error code to: %s.", strerror(errcode));

    jinue_info("Getting memory information from loader.");

    status = jinue_send(
        JINUE_DESC_LOADER_ENDPOINT,
        JINUE_MSG_GET_MEMINFO,
        &message,
        &errno,
        &errcode
    );

    if(status < 0) {
        if(errno == JINUE_EPROTO) {
            jinue_error("error: loader set error code to: %s.", strerror(errcode));
        } else {
            jinue_error("error: jinue_send() failed: %s.", strerror(errno));
        }

        return;
    }

    const jinue_loader_meminfo_t *meminfo = (jinue_loader_meminfo_t *)buffer;
    jinue_info("  Physical memory allocation address:   %#" PRIx64, meminfo->hint.physaddr);
    jinue_info("  Physical memory allocation limit:     %#" PRIx64, meminfo->hint.physlimit);
    jinue_info("  Extracted RAM disk address:           %#" PRIx64, meminfo->ramdisk.addr);
    jinue_info("  Extracted RAM disk size:              %#" PRIx64 " %" PRIu64,
        meminfo->ramdisk.size,
        meminfo->ramdisk.size
    );

    jinue_info("Blocking until loader exits.");

    status = jinue_send(
        JINUE_DESC_LOADER_ENDPOINT,
        JINUE_MSG_EXIT,
        &message,
        &errno,
        &errcode
    );

    if(status >= 0) {
        jinue_error("error: jinue_send() unexpectedly succeeded");
        return;
    }

    if(errno != JINUE_EIO) {
        jinue_error("error: jinue_send() failed: %s.", strerror(errno));
        return;
    }

    jinue_info("expected: jinue_send() set errno to: %s.", strerror(errno));
}

int main(int argc, char *argv[]) {
    /* Say hello. */
    jinue_info("Jinue test app (%s) started.", argv[0]);

    dump_cmdline_arguments(argc, argv);
    dump_environ();
    dump_auxvec();
    dump_syscall_implementation();

    dump_user_memory();

    dump_loader_memory();

    run_ipc_test();

    if(bool_getenv("DEBUG_DO_REBOOT")) {
        jinue_info("Rebooting.");
        jinue_reboot();
    }

    while (1) {
        jinue_yield_thread();
    }
    
    return EXIT_SUCCESS;
}
