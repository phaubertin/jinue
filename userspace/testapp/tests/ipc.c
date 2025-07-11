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
#include <jinue/utils.h>
#include <errno.h>
#include <internals.h>
#include <inttypes.h>
#include <limits.h>
#include <pthread.h>
#include <string.h>
#include "../utils.h"
#include "ipc.h"

#define MSG_FUNC_TEST   (JINUE_SYS_USER_BASE + 42)

int client_endpoint;

static void ipc_test_run_client(void) {
    /* The order of these buffers is shuffled on purpose because they will be
     * concatenated later and we don't want things to look OK by coincidence. */
    char reply2[5];
    char reply1[6];
    char reply3[40];

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

    intptr_t ret = jinue_send(client_endpoint, MSG_FUNC_TEST, &message, &errno, NULL);

    if(ret < 0) {
        jinue_error("error: jinue_send() failed: %s.", strerror(errno));
        return;
    }

    jinue_info("Client thread got reply from main thread:");
    jinue_info("  data:             \"%s%s%s\"", reply1, reply2, reply3);
    jinue_info("  size:             %" PRIuPTR, ret);

    jinue_info("Client thread is re-sending message.");

    message.recv_buffers        = NULL;
    message.recv_buffers_length = 0;

    ret = jinue_send(client_endpoint, MSG_FUNC_TEST, &message, &errno, NULL);

    if(ret >= 0) {
        jinue_error("error: jinue_send() unexpectedly succeeded");
        return;
    }

    if(errno != JINUE_EIO) {
        jinue_error("error: jinue_send() failed: %s.", strerror(errno));
        return;
    }

   jinue_info("expected: jinue_send() set errno to: %s.", strerror(errno));
}

static void *ipc_test_client_thread(void *arg) {
    jinue_info("Client thread is starting with argument: %#p", arg);
    
    ipc_test_run_client();

    jinue_info("Client thread is exiting.");

    return (void *)(uintptr_t)0xdeadbeef;
}

void run_ipc_test(void) {
    char recv_data[64];

    if(! bool_getenv("RUN_TEST_IPC")) {
        return;
    }

    jinue_info("Running threading and IPC test...");

    int endpoint = libc_allocate_descriptor();

    if(endpoint < 0) {
        jinue_error("error: libc_allocate_descriptor() failed: %s", strerror(errno));
        return;
    }

    int status = jinue_create_endpoint(endpoint, &errno);

    if(status < 0) {
        jinue_error("error: could not create IPC object: %s", strerror(errno));
        return;
    }

    client_endpoint = libc_allocate_descriptor();

    if(client_endpoint < 0) {
        jinue_error("error: libc_allocate_descriptor() failed: %s", strerror(errno));
        return;
    }

    status = jinue_mint(
        endpoint,
        JINUE_DESC_SELF_PROCESS,
        client_endpoint,
        JINUE_PERM_SEND,
        0xca11ab1e,
        &errno
    );

    if(status < 0) {
        jinue_error("error: jinue_mint() failed: %s", strerror(errno));
        return;
    }

    jinue_buffer_t recv_buffer;
    recv_buffer.addr = recv_data;
    recv_buffer.size = sizeof(recv_data);

    jinue_message_t message;
    message.recv_buffers        = &recv_buffer;
    message.recv_buffers_length = 1;

    jinue_info("Attempting to call jinue_receive() on the send-only descriptor.");

    intptr_t ret = jinue_receive(client_endpoint, &message, &errno);

    if(ret >= 0) {
        jinue_error("error: jinue_receive() unuexpectedly succeeded.");
        return;
    }

    if(errno != JINUE_EPERM) {
        jinue_error("error: jinue_receive() failed: %s.", strerror(errno));
        return;
    }

    jinue_info("expected: jinue_receive() set errno to: %s.", strerror(errno));

    pthread_attr_t attr;
    
    status = pthread_attr_init(&attr);

    if(status != 0) {
        jinue_error("error: pthread_attr_init() failed: %s", strerror(status));
        return;
    }

    status = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);

    if(status != 0) {
        jinue_error("error: pthread_attr_setstacksize() failed: %s", strerror(status));
        return;
    }

    pthread_t client_thread; 
    status = pthread_create(&client_thread, &attr, ipc_test_client_thread, (void *)(uintptr_t) 0xb01dface);

    if(status != 0) {
        jinue_error("error: could not create thread: %s", strerror(status));
        return;
    }

    ret = jinue_receive(endpoint, &message, &errno);

    if(ret < 0) {
        jinue_error("error: jinue_receive() failed: %s.", strerror(errno));
        return;
    }

    int function = message.recv_function;

    if(function != MSG_FUNC_TEST) {
        jinue_error("error: jinue_receive() unexpected function number: %i.", function);
        return;
    }

    jinue_info("Main thread received message:");
    jinue_info("  data:             \"%s\"", recv_data);
    jinue_info("  size:             %" PRIuPTR, ret);
    jinue_info("  function:         %u (user base + %u)", function, function - JINUE_SYS_USER_BASE);
    jinue_info("  cookie:           %#" PRIxPTR, message.recv_cookie);
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
        jinue_error("error: jinue_reply() failed: %s", strerror(errno));
        return;
    }

    jinue_info("Closing receiver descriptor.");

    status = jinue_close(endpoint, &errno);

    if(status < 0) {
        jinue_error("error: failed to close endpoint descriptor: %s", strerror(errno));
        return;
    }

    void *client_exit_value;
    status = pthread_join(client_thread, &client_exit_value);
    
    if(status != 0) {
        jinue_error("error: failed to join client thread: %s", strerror(status));
        return;
    }
    
    jinue_info("Client thread exit value is %#p.", client_exit_value);
    jinue_info("Main thread is running.");
}
