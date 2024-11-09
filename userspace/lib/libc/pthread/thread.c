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
#include <sys/mman.h>
#include <errno.h>
#include <internals.h>
#include <stdlib.h>
#include <pthread.h>
#include "attr.h"
#include "machine.h"
#include "thread.h"

static struct __pthread *pool = NULL;

static pthread_t get_thread_from_pool(void) {
    pthread_t thread = pool;

    if(thread != NULL) {
        pool = thread->next;
    }

    return thread;
}

static void free_thread_to_pool(pthread_t thread) {
    thread->next    = pool;
    pool            = thread;
}

static pthread_t allocate_thread(void) {
    pthread_t thread = get_thread_from_pool();

    if(thread != NULL) {
        return thread;
    }

    thread = malloc(sizeof(struct __pthread));

    if(thread == NULL) {
        return NULL;
    }

    int fd = __allocate_descriptor();

    if(fd < 0) {
        free(thread);
        return NULL;
    }

    int status = jinue_create_thread(fd, JINUE_DESC_SELF_PROCESS, &errno);

    if(status < 0) {
        __free_descriptor(fd);
        free(thread);
        return NULL;
    }

    thread->self    = thread;
    thread->fd      = fd;
    return thread;
}

static void *allocate_stack(size_t stacksize) {
    void *stack = mmap(NULL, stacksize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    return (stack == MAP_FAILED) ? NULL : stack;
}

static int setup_stack(pthread_t thread, const pthread_attr_t *attr) {
    if(__pthread_attr_has_stackaddr(attr)) {
        thread->stackaddr   = attr->stackaddr;
        thread->stacksize   = attr->stacksize;
        return 0;
    }

    if(thread->alloc_stacksize >= attr->stacksize) {
        thread->stackaddr = thread->alloc_stackaddr;
        thread->stacksize = thread->alloc_stacksize;
        return 0;
    }

    size_t stacksize    = (attr->stacksize + JINUE_PAGE_SIZE - 1) & ~JINUE_PAGE_MASK;
    void *stackaddr     = allocate_stack(stacksize);

    if(stackaddr == NULL) {
        return -1;
    }

    thread->alloc_stackaddr = stackaddr;
    thread->stackaddr       = stackaddr;
    thread->alloc_stacksize = stacksize;
    thread->stacksize       = stacksize;

    /* TODO unmap existing allocated stack (if not NULL, once system call exists) */
    
    return 0;
}

int pthread_create(
        pthread_t               *restrict thread,
        const pthread_attr_t    *restrict attr,
        void                    *(*start_routine)(void*),
        void                    *restrict arg) {
    
    if(attr == NULL) {
        attr = __pthread_attr_get_defaults();
    }

    pthread_t candidate = allocate_thread();

    if(candidate == NULL) {
        return errno;
    }

    int status = setup_stack(candidate, attr);

    if(status < 0) {
        free_thread_to_pool(candidate);
        return errno;
    }

    candidate->flags        = THREAD_FLAG_RUNNING;
    candidate->local_errno  = 0;

    if(attr->detachstate == PTHREAD_CREATE_DETACHED) {
        candidate->flags |= THREAD_FLAG_DETACHED;
    }

    status = jinue_start_thread(
        candidate->fd,
        __pthread_entry,
        __pthread_initialize_stack(candidate, start_routine, arg),
        &errno
    );

    if(status < 0) {
        free_thread_to_pool(candidate);
        return errno;
    }

    *thread = candidate;
    return 0;
}

int pthread_join(pthread_t thread, void **exit_status) {
    int status = jinue_join_thread(thread->fd, &errno);

    if(status < 0) {
        return errno;
    }

    *exit_status = thread->exit_status;
    return 0;
}

void pthread_exit(void *exit_status) {
    pthread_t thread    = pthread_self();
    thread->exit_status = exit_status;
    thread->flags       &= ~THREAD_FLAG_RUNNING;

    if(thread->flags & THREAD_FLAG_DETACHED) {
        free_thread_to_pool(thread);
    }
    
    jinue_exit_thread();
}
