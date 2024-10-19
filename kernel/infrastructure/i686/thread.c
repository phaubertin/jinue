/*
 * Copyright (C) 2019-2024 Philippe Aubertin.
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

#include <kernel/domain/alloc/page_alloc.h>
#include <kernel/infrastructure/i686/asm/thread.h>
#include <kernel/infrastructure/i686/cpu.h>
#include <kernel/infrastructure/i686/cpu_data.h>
#include <kernel/infrastructure/i686/descriptors.h>
#include <kernel/infrastructure/i686/types.h>
#include <kernel/interface/i686/trap.h>
#include <kernel/interface/i686/types.h>
#include <kernel/machine/thread.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>


/* defined in thread_switch.asm */
void thread_context_switch_stack(
        machine_thread_t *from_ctx,
        machine_thread_t *to_ctx,
        bool destroy_from);

/* For each thread, a page is allocated which contains:
 *  - The thread structure (thread_t), which includes the thread's message
 *    buffer; and
 *  - The thread's kernel stack.
 *
 * Switching thread context (see machine_switch_thread()) basically means
 * switching the kernel stack.
 *
 * The layout of this page is as follow:
 *
 *  +--------v-----------------v--------+ thread
 *  |                                   |  + (THREAD_CONTEXT_SIZE == PAGE_SIZE)
 *  |                                   |
 *  |                                   |
 *  |            Kernel stack           |
 *  |                                   |
 *  |                                   |
 *  |                                   |
 *  +-----------------------------------+ thread
 *  |                                   |  + sizeof(thread_t)
 *  |          Thread structure         |
 *  |             (thread_t)            |
 *  |                                   |
 *  +-----------------------------------+ thread
 *
 * The start of this page, and from there the thread structure, and kernel stack
 * base, can be found quickly by masking the least significant bits of the stack
 * pointer (with THREAD_CONTEXT_MASK).
 *
 * All machine-specific members of the thread structure (thread_t) are grouped
 * in the thread context (sub-)structure (machine_thread_t).
 * 
 */

static addr_t get_kernel_stack_base(machine_thread_t *thread_ctx) {
    thread_t *thread = (thread_t *)( (uintptr_t)thread_ctx & THREAD_CONTEXT_MASK );
    return (addr_t)thread + THREAD_CONTEXT_SIZE;
}

void machine_init_thread(thread_t *thread, const thread_params_t *params) {
    /* initialize fields */
    machine_thread_t *thread_ctx = &thread->thread_ctx;

    /* setup stack for initial return to user space */
    void *kernel_stack_base = get_kernel_stack_base(thread_ctx);

    trapframe_t *trapframe = (trapframe_t *)kernel_stack_base - 1;

    memset(trapframe, 0, sizeof(trapframe_t));

    trapframe->eip      = (uint32_t)params->entry;
    trapframe->esp      = (uint32_t)params->stack_addr;
    trapframe->eflags   = 2;
    trapframe->cs       = SEG_SELECTOR(GDT_USER_CODE, RPL_USER);
    trapframe->ss       = SEG_SELECTOR(GDT_USER_DATA, RPL_USER);
    trapframe->ds       = SEG_SELECTOR(GDT_USER_DATA, RPL_USER);
    trapframe->es       = SEG_SELECTOR(GDT_USER_DATA, RPL_USER);
    trapframe->fs       = SEG_SELECTOR(GDT_USER_DATA, RPL_USER);
    trapframe->gs       = SEG_SELECTOR(GDT_USER_DATA, RPL_USER);

    kernel_context_t *kernel_context = (kernel_context_t *)trapframe - 1;

    memset(kernel_context, 0, sizeof(kernel_context_t));

    /* This is the address to which thread_context_switch_stack() will return. */
    kernel_context->eip = (uint32_t)return_from_interrupt;

    /* set thread stack pointer */
    thread_ctx->saved_stack_pointer = (addr_t)kernel_context;
}

thread_t *machine_alloc_thread(void) {
    return page_alloc();
}

void machine_free_thread(thread_t *thread) {
    page_free(thread);
}

void machine_switch_thread(thread_t *from, thread_t *to, bool destroy_from) {
    /** ASSERTION: to argument must not be NULL */
    assert(to != NULL);
    
    /** ASSERTION: from argument must not be NULL if destroy_from is true */
    assert(from != NULL || !destroy_from);

    machine_thread_t *from_ctx  = (from == NULL) ? NULL : &from->thread_ctx;
    machine_thread_t *to_ctx    = &to->thread_ctx;

    /* setup TSS with kernel stack base for this thread context */
    addr_t kernel_stack_base = get_kernel_stack_base(to_ctx);
    tss_t *tss = get_tss();

    tss->esp0 = kernel_stack_base;
    tss->esp1 = kernel_stack_base;
    tss->esp2 = kernel_stack_base;

    /* update kernel stack address for SYSENTER instruction */
    if(cpu_has_feature(CPU_FEATURE_SYSENTER)) {
        wrmsr(MSR_IA32_SYSENTER_ESP, (uint64_t)(uintptr_t)kernel_stack_base);
    }

    /* switch thread context stack */
    thread_context_switch_stack(from_ctx, to_ctx, destroy_from);
}

thread_t *get_current_thread(void) {
    return (thread_t *)(get_esp() & THREAD_CONTEXT_MASK);
}
