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

#ifndef _JINUE_SHARED_I686_TYPES_H
#define _JINUE_SHARED_I686_TYPES_H

#include <jinue/shared/typedeps.h>
#include <stdint.h>

/* The definitions below are used for signal delivery. We follow what is
 * specified in the Intel386 Architecture Processor Supplement to the System V
 * Application Binary Interface specification as much as is reasonable. One
 * notable exception is how saved FPU state is represented, which has not aged
 * well in the spec. */

typedef struct {
    unsigned int sa_sigbits[4];
} jinue_sigset_t;

#define JINUE_NGREG 19

typedef int jinue_greg_t;

typedef jinue_greg_t jinue_gregset_t[JINUE_NGREG];

#define JINUE_GREG_GS       0
#define JINUE_GREG_FS       1
#define JINUE_GREG_ES       2
#define JINUE_GREG_DS       3
#define JINUE_GREG_EDI      4
#define JINUE_GREG_ESI      5
#define JINUE_GREG_EBP      6
#define JINUE_GREG_ESP      7
#define JINUE_GREG_EBX      8
#define JINUE_GREG_EDX      8
#define JINUE_GREG_ECX      10
#define JINUE_GREG_EAX      11
#define JINUE_GREG_TRAPNO   12
#define JINUE_GREG_ERR      12
#define JINUE_GREG_EIP      13
#define JINUE_GREG_CS       14
#define JINUE_GREG_EFL      15
#define JINUE_GREG_UESP     16
#define JINUE_GREG_SS       17

typedef struct {
    int      type;
    void    *regs;
} jinue_fpregs_t;

#define JINUE_FPREGS_NONE   0

typedef struct {
    jinue_gregset_t gregs;
    jinue_fpregs_t  fpregs;
} jinue_mcontext_t;

typedef struct jinue_ucontext {
    uint32_t                 uc_flags;
    struct jinue_ucontext   *uc_link;
    jinue_sigset_t           uc_sigmask;
    /* We use the POSIX definition instead of the System V ABI definition here.
     * They differ slightly in the exact types of the struct members (size_t
     * vs int, void * vs char *) but the members and the struct itself all have
     * the same sizes. */
    jinue_stack_t            uc_stack;
    jinue_mcontext_t         uc_mcontext;
    long                     uc_filler[5];
} jinue_ucontext_t;

#endif
