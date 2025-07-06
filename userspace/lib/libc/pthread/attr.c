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
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include "attr.h"

#define STACK_MAX           (512 * 1024 * 1024)

#define FLAG_HAS_STACKADDR  (1<<0)

static pthread_attr_t defaults = {
    .flags          = 0,
    .detachstate    = PTHREAD_CREATE_JOINABLE,
    .stacksize      = JINUE_STACK_SIZE,
    .stackaddr      = NULL
};

const pthread_attr_t *__pthread_attr_get_defaults(void) {
    return &defaults;
}

bool __pthread_attr_has_stackaddr(const pthread_attr_t *attr) {
    return !!(attr->flags & FLAG_HAS_STACKADDR);
}

int pthread_attr_destroy(pthread_attr_t *attr) {
    /* do nothing */
    return 0;
}

int pthread_attr_init(pthread_attr_t *attr) {
    memcpy(attr, &defaults, sizeof(defaults));
    return 0;
}

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate) {
    if(detachstate != NULL) {
        *detachstate = attr->detachstate;
    }
    return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate) {
    if(detachstate != PTHREAD_CREATE_DETACHED && detachstate != PTHREAD_CREATE_JOINABLE) {
        return EINVAL;
    }

    attr->detachstate = detachstate;
    return 0;
}

int pthread_attr_getstacksize(const pthread_attr_t *restrict attr, size_t *restrict stacksize) {
    if(stacksize != NULL) {
        *stacksize = attr->stacksize;
    }
    return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize) {
    if(stacksize < PTHREAD_STACK_MIN || stacksize > STACK_MAX) {
        return EINVAL;
    }

    attr->stacksize = stacksize;
    return 0;
}

int pthread_attr_getstack(
        const pthread_attr_t     *restrict attr,
        void                    **restrict stackaddr,
        size_t                   *restrict stacksize) {
    
    if(stackaddr != NULL) {
        *stackaddr = attr->stackaddr;
    }

    if(stacksize != NULL) {
        *stacksize = attr->stacksize;
    }

    return 0;
}

int pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr, size_t stacksize) {
    if(stacksize < PTHREAD_STACK_MIN || stacksize > STACK_MAX) {
        return EINVAL;
    }

    if((uintptr_t)stackaddr % PAGE_SIZE != 0) {
        return EINVAL;
    }

    attr->stacksize = stacksize;
    attr->stackaddr = stackaddr;
    attr->flags     |= FLAG_HAS_STACKADDR;
    return 0;
}
