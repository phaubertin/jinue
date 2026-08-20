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
#include "tests.h"

#define THREAD_EXIT_VALUE ((void *)0xdeadbeef)

static void cleanup_routine(void *str) {
    jinue_info("Running cancellation handler: %s", str);
}

static void inner_func(void) {
    pthread_cleanup_push(cleanup_routine, "inner handler");

    pthread_exit(THREAD_EXIT_VALUE);

    pthread_cleanup_pop(0);
    
}

static void *thread_func(void *arg) {
    jinue_info("Thread started");

    pthread_cleanup_push(cleanup_routine, "outer handler");

    inner_func();

    pthread_cleanup_pop(0);

    return NULL;
}

void run_exit_thread_test(void) {
    if(! bool_getenv("RUN_TEST_EXIT_THREAD")) {
        return;
    }

    jinue_info("Running thread exit test...");

    pthread_t thread;
    int status = start_thread(&thread, thread_func, NULL);

    if(status != EXIT_SUCCESS) {
        /* start_thread() does the error logging. */
        return;
    }

    jinue_info("Joining the thread...");

    void *thread_exit_value;
    status = pthread_join(thread, &thread_exit_value);
    
    if(status != 0) {
        jinue_error("error: failed to join the thread: %s", strerror(status));
        return;
    }

    if(thread_exit_value == THREAD_EXIT_VALUE) {
        jinue_info("Thread exit value is %#p", thread_exit_value);
    }
    else {
        jinue_error("error: unexpected thread exit value is %#p", thread_exit_value);
        return;
    }

    jinue_info("Main thread is running.");
}
