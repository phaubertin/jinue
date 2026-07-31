/*
 * Copyright (C) 2019-2026 Philippe Aubertin.
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

#ifndef JINUE_KERNEL_INTERFACE_I686_EXPORTS_TYPES_H
#define JINUE_KERNEL_INTERFACE_I686_EXPORTS_TYPES_H

#include <stdint.h>

typedef struct {
    /* The following four registers are the system call arguments. */
    uint32_t    eax;
    uint32_t    ebx;
    uint32_t    esi;
    uint32_t    edi;
    uint32_t    edx;
    uint32_t    ecx;
    uint32_t    ds;
    uint32_t    es;
    uint32_t    fs;
    uint32_t    gs;
    uint32_t    errcode;
    uint32_t    trapno;
    uint32_t    ebp;
    uint32_t    eip;
    uint32_t    cs;
    uint32_t    eflags;
    /* Caution: the two fields below are only populated (by the CPU itself)
     * when we trap from user space. Do not try to access when the trap comes
     * from kernel space. */
    uint32_t    esp;
    uint32_t    ss;
} trapframe_t;

#endif
