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

#include <kernel/infrastructure/i686/asm/x86.h>
#include <kernel/infrastructure/i686/isa/regs.h>
#include <kernel/infrastructure/i686/cpuinfo.h>
#include <kernel/infrastructure/i686/fpu.h>
#include <stdint.h>

/** Initialize the FPU for x87 and SSE instructions */
void initialize_fpu(void) {
    /* No need to check for FPU since this is part of CPU requirements for this
     * kernel. */
    uint32_t cr0 = get_cr0();

    /* no FPU emulation */
    cr0 &= ~X86_CR0_EM;

    /* WAIT/FWAIT will trap when TS is set, just like other FPU instructions. */
    cr0 |= X86_CR0_MP;

    /* Task not switched. */
    cr0 &= ~X86_CR0_TS;

    /* Use internally-generated exceptions for FPU errors, */
    cr0 |= X86_CR0_NE;

    set_cr0(cr0);

    if(!cpu_has_feature(CPU_FEATURE_SSE)) {
        return;
    }

    uint32_t cr4 = get_cr4();

    /* Enable SSE (we support saving and restoring the state). */
    cr4 |= X86_CR4_OSFXSR;

    /* We support SIMD floating point exceptions */
    cr4 |= X86_CR4_OSXMMEXCPT;

    set_cr4(cr4);
}
