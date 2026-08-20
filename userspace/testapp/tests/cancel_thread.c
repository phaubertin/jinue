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
#include <jinue/utils.h>
#include <errno.h>
#include <internals.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../utils.h"
#include "cancel_thread.h"

#define MSG_SYNCHRONIZE JINUE_SYS_USER_BASE

static int endpoint;

static void cleanup_routine(void *str) {
    jinue_info("Running cancellation handler: %s", str);
}

static void inner_func(void) {
    pthread_cleanup_push(cleanup_routine, "inner handler");

    pthread_testcancel();

    int oldstate;
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);

    if(oldstate != PTHREAD_CANCEL_ENABLE) {
        jinue_warning("warning: pthread_setcancelstate(): oldstate was not PTHREAD_CANCEL_ENABLE");
    }

    jinue_message_t message;
    message.send_buffers_length = 0;
    message.recv_buffers_length = 0;

    intptr_t ret = jinue_send(endpoint, MSG_SYNCHRONIZE, &message, &errno, NULL);

    if(ret < 0) {
        jinue_error("error: jinue_send() failed: %s.", strerror(errno));
    }

    for(int count = 1; true; ++count) {
        /* This loop should log at least two iterations:
         *  - On the first iteration, the cancellability state is still
         *    disabled when pthread_testcancel() is called.
         *  - On the second iteration, pthread_testcancel() is called after
         *    logging. */
        jinue_info("Thread loop iteration: %d", count);

        pthread_testcancel();

        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);

        if(oldstate != (count == 1 ? PTHREAD_CANCEL_DISABLE : PTHREAD_CANCEL_ENABLE)) {
            jinue_warning(
                "warning: pthread_setcancelstate(): oldstate was not %s",
                count == 1 ? "PTHREAD_CANCEL_DISABLE" : "PTHREAD_CANCEL_ENABLE"
            );
        }

        jinue_yield_thread();
    }

    pthread_cleanup_pop(0);
    
}

static void *thread_func(void *arg) {
    jinue_info("Thread started");

    pthread_cleanup_push(cleanup_routine, "outer handler");

    inner_func();

    pthread_cleanup_pop(0);

    return NULL;
}

void run_cancel_thread_test(void) {
    if(! bool_getenv("RUN_TEST_CANCEL_THREAD")) {
        return;
    }

    jinue_info("Running thread cancellation test...");

    endpoint = libc_allocate_descriptor();
    int status = jinue_create_endpoint(endpoint, &errno);

    if(status < 0) {
        jinue_error("error: could not create IPC endpoint: %s", strerror(errno));
        return;
    }

    pthread_t thread;
    status = start_thread(&thread, thread_func, NULL);

    if(status != EXIT_SUCCESS) {
        /* start_thread() does the error logging. */
        return;
    }

    /* Synchronize with the other thread to give it a chance to disable its
     * cancellability before we try to cancel it. */
    jinue_info("Waiting for the thread to start...");

    jinue_message_t message;
    message.recv_buffers_length = 0;

    intptr_t ret = jinue_receive(endpoint, &message, &errno);

    if(ret < 0) {
        jinue_error("error: jinue_receive() failed: %s.", strerror(errno));
        return;
    }

    jinue_message_t reply;
    reply.send_buffers_length = 0;

    ret = jinue_reply(&reply, &errno);

    if(ret < 0) {
        jinue_error("error: jinue_reply() failed: %s", strerror(errno));
        return;
    }

    jinue_info("Cancelling the thread...");

    status = pthread_cancel(thread);

    if(status < 0) {
        jinue_error("error: could not cancel thread: %s", strerror(errno));
        return;
    }

    jinue_info("Joining the thread...");

    void *thread_exit_value;
    status = pthread_join(thread, &thread_exit_value);
    
    if(status != 0) {
        jinue_error("error: failed to join the thread: %s", strerror(status));
        return;
    }

    if(thread_exit_value == PTHREAD_CANCELED) {
        jinue_info("Thread exit value is PTHREAD_CANCELED");
    }
    else {
        jinue_error("error: unexpected thread exit value is %#p", thread_exit_value);
        return;
    }

    jinue_info("Main thread is running.");
}
