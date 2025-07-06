/*
 * Copyright (C) 2025 Philippe Aubertin.
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

#include <jinue/utils.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "../utils.h"
#include "abcd.h"

#define THREADS_NUM 8

static void *thread_func(void *arg) {
    while(true) {
        jinue_info("%s", (const char *)arg);
    }

    return NULL;
}

bool start_thread(pthread_t *thread, const char *str) {
    pthread_attr_t attr;
    
    int status = pthread_attr_init(&attr);

    if(status != 0) {
        jinue_error("error: pthread_attr_init() failed: %s", strerror(status));
        return EXIT_FAILURE;
    }

    status = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);

    if(status != 0) {
        jinue_error("error: pthread_attr_setstacksize() failed: %s", strerror(status));
        return EXIT_FAILURE;
    }

    status = pthread_create(thread, &attr, thread_func, (char *)str);

    if(status != 0) {
        jinue_error("error: could not create thread: %s", strerror(status));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static void initialize_string(char *str, int thread_index) {
    for(int idx = 0; idx < THREADS_NUM; ++idx) {
        str[2 * idx]        = '.';
        str[2 * idx + 1]    = ' ';
    }

    str[2 * thread_index]   = 'A' + thread_index;
    str[2 * THREADS_NUM]    = '\0';
}

void run_abcd_test(void) {
    pthread_t threads[THREADS_NUM];
    char strings[THREADS_NUM][2 * THREADS_NUM + 1];

    if(! bool_getenv("RUN_TEST_ABCD")) {
        return;
    }

    for(int idx = 0; idx < THREADS_NUM; ++idx) {
        initialize_string(strings[idx], idx);
        int status = start_thread(&threads[idx], strings[idx]);

        if(status != EXIT_SUCCESS) {
            return;
        }
    }

    for(int idx = 0; idx < THREADS_NUM; ++idx) {
        void *client_exit_value;
        pthread_join(threads[idx], &client_exit_value);
    }
}
