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

#include <jinue/jinue.h>
#include <jinue/threads.h>
#include <sys/mman.h>
#include <errno.h>

struct jinue_thread {
    /* The first two members must be in this order and at the start of the
     * structure, i.e. they must be at the top of the stack when the thread
     * starts. jinue_thread_entry() relies on this. */
    void    *(*start_routine)(void*);
    void    *arg_and_retval;
    int      fd;
};

int jinue_thread_init(jinue_thread_t *thread, int fd, size_t stacksize) {
    int status = jinue_create_thread(fd, JINUE_DESC_SELF_PROCESS, &errno);

    if(status < 0) {
        return status;
    }

    char *stack = mmap(
            NULL,
            stacksize,
            JINUE_PROT_READ | JINUE_PROT_WRITE,
            MAP_SHARED | MAP_ANONYMOUS,
            -1,
            0
    );

    if(stack == MAP_FAILED) {
        jinue_close(fd, NULL);
        return -1;
    }

    char *stackbase = stack + stacksize;
    *thread = (struct jinue_thread *)stackbase - 1;

    (*thread)->fd = fd;

    return 0;
}

int jinue_thread_start(jinue_thread_t thread, void *(*start_routine)(void*), void *arg) {
    thread->start_routine   = start_routine;
    thread->arg_and_retval  = arg;
    return jinue_start_thread(thread->fd, jinue_thread_entry, &thread->start_routine, &errno);
}

int jinue_thread_join(jinue_thread_t thread, void **value_ptr) {
    int status = jinue_join_thread(thread->fd, &errno);

    if(status < 0) {
        return status;
    }

    *value_ptr = thread->arg_and_retval;

    return 0;
}

int jinue_thread_destroy(jinue_thread_t thread) {
    /* TODO dealloc/unmap stack */
    return jinue_close(thread->fd, &errno);
}
