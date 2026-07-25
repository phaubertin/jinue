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

#include <jinue/utils.h>
#include <sys/mman.h>
#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "../utils.h"
#include "sse.h"

#define PASS            0

#define FAIL            1

#define SIZEOF_SSE_WORD 16

typedef struct {
    pthread_t    pthread;
    const void  *tbuffer;
    void        *result;
} thread_context_t;

static void *thread_func(void *arg) {
    const thread_context_t *ctx = arg;
    compute_sse(ctx->tbuffer, ctx->result);
    return NULL;
}

static int do_run_test(void) {
    thread_context_t threads[SSE_TEST_NUM_THREADS];

    /* ------------------------------------------------------------------
     * Step 1: allocate memory
     * ------------------------------------------------------------------ */
    /* We need these allocations to be aligned on a 16-bit boundary for the SSE
     * instructions. mmap() returns memory aligned on a page boundary. */
#define SIZEOF_BUFFER (SSE_TEST_NUM_THREADS * SSE_TEST_PER_THREAD * SIZEOF_SSE_WORD)
    unsigned char *buffer = mmap(
        NULL,
        SIZEOF_BUFFER,
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0
    );

    if(buffer == MAP_FAILED) {
        jinue_error("Memory allocation error (buffer)");
        return FAIL;
    }

    unsigned char *expected = mmap(
        NULL,
        PAGE_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0
    );

    if(expected == MAP_FAILED) {
        jinue_error("Memory allocation error (expected)");
        return FAIL;
    }

#define SIZEOF_RESULT (SSE_TEST_NUM_THREADS * SIZEOF_SSE_WORD)
    unsigned char *actual = expected + SIZEOF_RESULT;

    /* ------------------------------------------------------------------
     * Step 2: initialize buffer
     * ------------------------------------------------------------------ */
    
    for(int idx = 0; idx < SIZEOF_BUFFER; ++idx) {
        buffer[idx] = (unsigned char)rand();
    }

    /* ------------------------------------------------------------------
     * Step 3: compute expectation
     * ------------------------------------------------------------------ */

    for(int tid = 0; tid < SSE_TEST_NUM_THREADS; ++tid) {
        const uint32_t *tbuffer = (uint32_t *)&buffer[tid * SSE_TEST_PER_THREAD * SIZEOF_SSE_WORD];
        uint32_t *results = (uint32_t *)&expected[tid * SIZEOF_SSE_WORD];

        const size_t per_sse_word = SIZEOF_SSE_WORD / sizeof(uint32_t);

        for(int offset = 0; offset < per_sse_word; ++offset) {
            results[offset] = 0;

            for(int idx = 0; idx < SSE_TEST_PER_THREAD; ++idx) {
                results[offset] ^= tbuffer[idx * per_sse_word + offset];
            }
        }
    }

    /* ------------------------------------------------------------------
     * Step 4: compute actual result with SSE assembly code
     * ------------------------------------------------------------------ */

    /* We split the work between multiple threads to test FPU/SSE state save and
     * restore by the kernel and ensure each thread's state remains isolated.
     * The assembly code that computes the result periodically calls
     * jinue_yield_thread() to force context switching between threads. */

    for(int tid = 0; tid < SSE_TEST_NUM_THREADS; ++tid) {
        thread_context_t *ctx = &threads[tid];
        ctx->tbuffer    = &buffer[tid * SSE_TEST_PER_THREAD * SIZEOF_SSE_WORD];
        ctx->result     = &actual[tid * SIZEOF_SSE_WORD];

        int status = start_thread(&(ctx->pthread), thread_func, ctx);

        if(status != EXIT_SUCCESS) {
            return FAIL;
        }
    }

    for(int tid = 0; tid < SSE_TEST_NUM_THREADS; ++tid) {
        pthread_join(threads[tid].pthread, NULL);
    }

    /* ------------------------------------------------------------------
     * Final step: compare results
     * ------------------------------------------------------------------ */

    if(memcmp(expected, actual, SIZEOF_RESULT) != 0) {
        return FAIL;
    }

    return PASS;
}

void run_sse_test(void) {
    if(! bool_getenv("RUN_TEST_SSE")) {
        return;
    }

    int result = do_run_test();
    jinue_info("SSE test result: %s", result == PASS ? "PASS" : "FAIL");
}
