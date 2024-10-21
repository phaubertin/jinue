/*
 * Copyright (C) 2019-2023 Philippe Aubertin.
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
#include "../machine.h"
#include "stubs.h"

static jinue_syscall_stub_t syscall_stubs[] = {
        [JINUE_I686_HOWSYSCALL_INTERRUPT]    = jinue_syscall_intr,
        [JINUE_I686_HOWSYSCALL_FAST_AMD]     = jinue_syscall_fast_amd,
        [JINUE_I686_HOWSYSCALL_FAST_INTEL]   = jinue_syscall_fast_intel
};

static int syscall_stub_index = JINUE_I686_HOWSYSCALL_INTERRUPT;

int jinue_init(int implementation, int *perrno) {
    if(implementation < 0 || implementation > JINUE_I686_HOWSYSCALL_LAST) {
        *perrno = JINUE_EINVAL;
        return -1;
    }

    syscall_stub_index = implementation;
    return 0;
}

uintptr_t jinue_syscall(jinue_syscall_args_t *args) {
    return syscall_stubs[syscall_stub_index](args);
}

static inline void jinue_set_errno(int *perrno, int errval) {
    if(perrno != NULL) {
        *perrno = errval;
    }
}

intptr_t jinue_syscall_with_usual_convention(jinue_syscall_args_t *args, int *perrno) {
    const intptr_t retval = (intptr_t)jinue_syscall(args);

    if(retval < 0) {
        jinue_set_errno(perrno, args->arg1);
    }

    return retval;
}
