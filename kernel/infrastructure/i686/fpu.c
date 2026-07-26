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

#include <kernel/domain/services/logging.h>
#include <kernel/infrastructure/i686/asm/x86.h>
#include <kernel/infrastructure/i686/asm/thread.h>
#include <kernel/infrastructure/i686/isa/instrs.h>
#include <kernel/infrastructure/i686/isa/regs.h>
#include <kernel/infrastructure/i686/cpuinfo.h>
#include <kernel/infrastructure/i686/fpu.h>
#include <kernel/infrastructure/i686/thread.h>
#include <kernel/machine/thread.h>
#include <stdint.h>
#include <string.h>

/* Mask all SSE exceptions, round to nearest (even), clear status flags. */
#define DEFAULT_MXCSR   0x1f80

/* Same value as set by FINIT/FNINIT:
 * Mask all x87 exceptions, round to nearest (even), use 64-bit precision. */
#define DEFAULT_X87_CW  0x037f

typedef struct {
    uint32_t control_word;
    uint32_t status_word;
    uint32_t tag_word;
    /* We don't care about the rest. */
} fsave_t;

typedef struct {
    uint16_t control_word;
    uint16_t status_word;
    uint8_t tag_word;
    uint8_t reserved1;
    uint16_t fop;
    uint32_t fip_l;
    uint16_t fip_h;
    uint16_t reserved2;
    uint32_t fdp_l;
    uint16_t fdp_h;
    uint16_t reserved3;
    uint32_t mxcsr;
    uint32_t mxcsr_mask;
    /* We don't care about the rest. */
} fxsave_t;

/** value of MXCSR_MASK set by FXSAVE instruction */
uint32_t mxcsr_mask;

/**
 * Read the value of MXCSR_MASK
 * 
 * The Intel SDM requires us to actually call the FXSAVE instruction to
 * determine the value of MXCSR_MASK. See volume 1 section 11.6.6
 * "Guidelines for Writing to the MXCSR Register".
 *
 */
static void read_mxcsr_mask(void) {
    fninit();
    ldmxcsr(DEFAULT_MXCSR);

    /* 512 bytes plus padding to allow to re-align on a 16 bytes boundary. */
    unsigned char buffer[512 + 15];
    fxsave_t* area = ALIGN_END_PTR(buffer, 16);

    area->mxcsr_mask = 0;

    fxsave(area);

    mxcsr_mask = area->mxcsr_mask;

    if (mxcsr_mask == 0) {
        mxcsr_mask = 0x0000ffbf;
    }
}

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

    /* Use internally-generated exceptions for FPU errors, not the legacy
     * external interrupt mechanism. */
    cr0 |= X86_CR0_NE;

    set_cr0(cr0);

    if(!cpu_has_feature(CPU_FEATURE_SSE)) {
        info("Streaming SIMD Extensions (SSE) are not supported.");
        return;
    }

    info("Enabling Streaming SIMD Extensions (SSE).");

    uint32_t cr4 = get_cr4();

    /* Enable SSE (we support saving and restoring the state). */
    cr4 |= X86_CR4_OSFXSR;

    /* We support SIMD floating point exceptions */
    cr4 |= X86_CR4_OSXMMEXCPT;

    set_cr4(cr4);

    read_mxcsr_mask();
}

/**
 * Initialize a thread's FPU save/restore area
 * 
 * @param thread the thread
 */
void prepare_fpu_area(thread_t *thread) {
    void *area = get_thread_fpu_area(thread);
    memset(area, 0, THREAD_FPU_AREA_SIZE);

    if(cpu_has_feature(CPU_FEATURE_FXSR)) {
        fxsave_t *fxsave_area = area;
        fxsave_area->control_word   = DEFAULT_X87_CW;
        /* all empty */
        fxsave_area->tag_word       = 0;
        /* Mask all SSE exceptions, round to nearest (even), clear status flags. */
        fxsave_area->mxcsr          = DEFAULT_MXCSR;
        fxsave_area->mxcsr_mask     = mxcsr_mask;
    }
    else {
        fsave_t *fsave_area = area;
        /* Same value as set by FINIT/FNINIT:
         * Mask all x87 exceptions, round to nearest (even), use 64-bit precision. */
        fsave_area->control_word    = DEFAULT_X87_CW;
        /* all empty */
        fsave_area->tag_word        = 0xffff;
    }
}

/**
 * Set a thread as using the FPU
 * 
 * We use a lazy initialization scheme where the very first time a thread uses
 * the FPU, it is marked as using the FPU, which makes it subject to FPU state
 * save/restore on context switches. If a thread never uses the FPU, it's FPU
 * state never has to be saved or restored.
 * 
 * CVE-2018-3665: recent Intel CPUs leak FPU state through speculative
 * execution. On these CPUs, if a thread isn't using the FPU, we always restore
 * the FPU state after a context switch but never save it (we "restore" the
 * initialization state).
 * 
 * CPU errata ICL012, ICL013, ICL017: some Intel CPUs mistakenly trigger a #NM
 * exception in situations where they should trigger a #UD exception instead.
 * This function, which is called by the exception handler, detects the
 * situation and lets the caller know.
 * 
 * @param thread the thread
 * @return true on success, false if thread was already marked as using the FPU
 */
bool use_fpu(thread_t *thread) {
    machine_thread_t *machine_thread = &thread->machine_thread;

    if(machine_thread->flags & THREAD_FLAG_USES_FPU) {
        return false;
    }

    /* Thread now uses the FPU. */
    machine_thread->flags |= THREAD_FLAG_USES_FPU;

    /* Make sure the FPU is initialized with the initial state set by
     * prepare_fpu_area(). */
    machine_thread->flags |= THREAD_FLAG_FPU_STATE_SAVED;

    return true;
}

/**
 * Save the FPU state of a thread
 * 
 * @param thread the thread
 */
void save_fpu_state(thread_t *thread) {
    machine_thread_t *machine_thread = &thread->machine_thread;

    const bool uses_fpu = !!(machine_thread->flags & THREAD_FLAG_USES_FPU);
    const bool has_saved_state = !!(machine_thread->flags & THREAD_FLAG_FPU_STATE_SAVED);
    const bool has_cve = cpu_needs_workaround(CPU_WORKAROUND_CVE2018_3665);

    if(has_cve) {
        /* CVE-2018-3665 mitigation: force state to be restored even if FPU is
         * not in use to prevent speculation-based information leak attacks. */
        machine_thread->flags |= THREAD_FLAG_FPU_STATE_SAVED;
    }

    if(has_saved_state || !uses_fpu) {
        return;
    }

    if(cpu_has_feature(CPU_FEATURE_FXSR)) {
        fxsave(get_thread_fpu_area(thread));
    }
    else {
        fnsave(get_thread_fpu_area(thread));
    }

    /* state saved */
    machine_thread->flags |= THREAD_FLAG_FPU_STATE_SAVED;
}

/** Restore the FPU state of the current thread */
void restore_fpu_state(void) {
    thread_t *thread = get_current_thread();
    machine_thread_t *machine_thread = &thread->machine_thread;

    const bool uses_fpu = !!(machine_thread->flags & THREAD_FLAG_USES_FPU);
    const bool has_saved_state = !!(machine_thread->flags & THREAD_FLAG_FPU_STATE_SAVED);

    if(uses_fpu) {
        clts();
    } else {
        /* If the thread is not yet marked as using the FPU, we want to set the
         * TS flag to cause a trap when it does so for the first time. Note
         * that a thread not using the FPU (uses_fpu false) does not imply it
         * does not have saved state to restore (has_saved_state false) because
         * of the CVE-2018-3665 mitigation. */
        uint32_t cr0 = get_cr0();
        cr0 |= X86_CR0_TS;
        set_cr0(cr0);
    }

    if(!has_saved_state) {
        return;
    }

    if(cpu_has_feature(CPU_FEATURE_FXSR)) {
        fxrstor(get_thread_fpu_area(thread));
    }
    else {
        frstor(get_thread_fpu_area(thread));
    }

    /* saved state consumed */
    machine_thread->flags &= ~THREAD_FLAG_FPU_STATE_SAVED;
}
