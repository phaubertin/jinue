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

#ifndef JINUE_KERNEL_INTERFACE_I686_EXPORTS_TRAP_H
#define JINUE_KERNEL_INTERFACE_I686_EXPORTS_TRAP_H

#include <kernel/interface/i686/exports/types.h>
#include <stdint.h>

static inline uint32_t *msg_arg0_ptr(trapframe_t *trapframe) {
    return &trapframe->eax;
}

static inline uint32_t *msg_arg1_ptr(trapframe_t *trapframe) {
    return &trapframe->ebx;
}

static inline uint32_t *msg_arg2_ptr(trapframe_t *trapframe) {
    return &trapframe->esi;
}

static inline uint32_t *msg_arg3_ptr(trapframe_t *trapframe) {
    return &trapframe->edi;
}

#define msg_arg0(tf) (*msg_arg0_ptr(tf))

#define msg_arg1(tf) (*msg_arg1_ptr(tf))

#define msg_arg2(tf) (*msg_arg2_ptr(tf))

#define msg_arg3(tf) (*msg_arg3_ptr(tf))

#endif
