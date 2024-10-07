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
#include <jinue/util.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include "../util.h"

#define THREAD_STACK_SIZE   4096

#define MSG_FUNC_TEST       (JINUE_SYS_USER_BASE + 42)

#define IPC_DESCRIPTOR      0

#define CLIENT_DESCRIPTOR   1

static char ipc_test_thread_stack[THREAD_STACK_SIZE];

static void ipc_test_run_client(void) {
    /* The order of these buffers is shuffled on purpose because they will be
     * concatenated later and we don't want things to look OK by coincidence. */
    char reply2[5];
    char reply1[6];
    char reply3[40];
    int errno;

    const char hello[] = "Hello ";
    const char world[] = "World!";

    jinue_info("Client thread is sending message.");

    jinue_const_buffer_t send_buffers[2];
    send_buffers[0].addr = hello;
    send_buffers[0].size = sizeof(hello) - 1;   /* do not include NUL terminator */
    send_buffers[1].addr = world;
    send_buffers[1].size = sizeof(world);       /* includes NUL terminator */

    jinue_buffer_t reply_buffers[3];
    reply_buffers[0].addr = reply1;
    reply_buffers[0].size = sizeof(reply1) - 1; /* minus one chunk is NUL terminated */
    reply_buffers[1].addr = reply2;
    reply_buffers[1].size = sizeof(reply2) - 1; /* minus one chunk is NUL terminated */
    reply_buffers[2].addr = reply3;
    reply_buffers[2].size = sizeof(reply3);     /* final NUL is part of the reply message */

    memset(reply1, 0, sizeof(reply1));
    memset(reply2, 0, sizeof(reply2));
    memset(reply3, 0, sizeof(reply3));

    jinue_message_t message;
    message.send_buffers        = send_buffers;
    message.send_buffers_length = 2;
    message.recv_buffers        = reply_buffers;
    message.recv_buffers_length = 3;

    intptr_t ret = jinue_send(CLIENT_DESCRIPTOR, MSG_FUNC_TEST, &message, &errno);

    if(ret < 0) {
        jinue_error("jinue_send() failed with error: %s.", strerror(errno));
        return;
    }

    jinue_info("Client thread got reply from main thread:");
    jinue_info("  data:             \"%s%s%s\"", reply1, reply2, reply3);
    jinue_info("  size:             %" PRIuPTR, ret);
}

static void ipc_test_client_thread(void) {
    ipc_test_run_client();

    jinue_info("Client thread is exiting.");

    jinue_exit_thread();
}

void run_ipc_test(void) {
    char recv_data[64];

    if(! bool_getenv("RUN_TEST_IPC")) {
        return;
    }

    jinue_info("Running threading and IPC test...");

    int status = jinue_create_ipc(IPC_DESCRIPTOR, &errno);

    if(status < 0) {
        jinue_error("Could not create IPC object: %s", strerror(errno));
        return;
    }

    status = jinue_dup(JINUE_SELF_PROCESS_DESCRIPTOR, IPC_DESCRIPTOR, CLIENT_DESCRIPTOR, &errno);

    if(status < 0) {
        jinue_error("jinue_dup() failed: %s", strerror(errno));
        return;
    }

    status = jinue_create_thread(
        JINUE_SELF_PROCESS_DESCRIPTOR,
        ipc_test_client_thread,
        &ipc_test_thread_stack[THREAD_STACK_SIZE],
        &errno
    );

    if (status != 0) {
        jinue_error("Could not create thread: %s", strerror(errno));
        return;
    }

    jinue_buffer_t recv_buffer;
    recv_buffer.addr = recv_data;
    recv_buffer.size = sizeof(recv_data);

    jinue_message_t message;
    message.recv_buffers        = &recv_buffer;
    message.recv_buffers_length = 1;

    intptr_t ret = jinue_receive(IPC_DESCRIPTOR, &message, &errno);

    if(ret < 0) {
        jinue_error("jinue_receive() failed with error: %s.", strerror(errno));
        return;
    }

    int function = message.recv_function;

    if(function != MSG_FUNC_TEST) {
        jinue_error("jinue_receive() unexpected function number: %i.", function);
        return;
    }

    jinue_info("Main thread received message:");
    jinue_info("  data:             \"%s\"", recv_data);
    jinue_info("  size:             %" PRIuPTR, ret);
    jinue_info("  function:         %u (user base + %u)", function, function - JINUE_SYS_USER_BASE);
    jinue_info("  cookie:           %" PRIuPTR, message.recv_cookie);
    jinue_info("  reply max. size:  %" PRIuPTR, message.reply_max_size);

    const char reply_string[] = "Hi, Main Thread!";

    jinue_const_buffer_t reply_buffer;
    reply_buffer.addr = reply_string;
    reply_buffer.size = sizeof(reply_string);   /* includes NUL terminator */

    jinue_message_t reply;
    reply.send_buffers          = &reply_buffer;
    reply.send_buffers_length   = 1;

    ret = jinue_reply(&reply, &errno);

    if(ret < 0) {
        jinue_error("jinue_reply() failed with error: %s", strerror(errno));
        return;
    }

    jinue_info("Main thread is running.");
    jinue_info("Test passed.");
}
