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

#include <jinue/shared/types.h>
#include <kernel/interface/machine/signal.h>
#include <kernel/machine/thread.h>
#include <kernel/types.h>
#include <stdint.h>

void deliver_signal(trapframe_t *trapframe, int signo, uint32_t sigmask) {

    /* TODO make sure we are not writing where the user would not be allowed
     * to. */

    unsigned char *stack = (unsigned char *)trapframe->esp;
#define push(t, a) stack = (a == 0) ? stack - sizeof(t) : ALIGN_START_PTR(stack - sizeof(t), 16)
    
    push(jinue_ucontext_t, 16);
    jinue_ucontext_t *ucontext = (jinue_ucontext_t *)stack;
    
    push(jinue_siginfo_t, 16);
    jinue_siginfo_t *siginfo = (jinue_siginfo_t *)stack;

    /* handler arguments */
    push(jinue_ucontext_t *, 0);
    *(jinue_ucontext_t **)stack = ucontext;
    push(jinue_siginfo_t *, 0);
    *(jinue_siginfo_t **)stack = siginfo;
    push(int, 0);
    *(int *)stack = signo;

    /* NULL return address: the handler has the responsibility to call the
     * appropriate system call when it is done and shouldn't just return. */
    push(void *, 0);
    *(void **)stack = NULL;

    const thread_t *thread = get_current_thread();
    const process_t *process = thread->process;
    
    trapframe->esp = (uintptr_t)stack;
    trapframe->eip = (uintptr_t)process->signal_handler;

    ucontext->uc_flags = 0;
    ucontext->uc_link = NULL;

    ucontext->uc_sigmask.sa_sigbits[0] = sigmask;
    /* These are unused. */
    ucontext->uc_sigmask.sa_sigbits[1] = 0;
    ucontext->uc_sigmask.sa_sigbits[2] = 0;
    ucontext->uc_sigmask.sa_sigbits[3] = 0;
    
    /* unused for now */
    ucontext->uc_stack.ss_flags = 0;
    ucontext->uc_stack.ss_size = 0;
    ucontext->uc_stack.ss_sp = 0;

    jinue_mcontext_t *mcontext = &ucontext->uc_mcontext;

    mcontext->gregs[JINUE_GREG_GS] = trapframe->gs;
    mcontext->gregs[JINUE_GREG_FS] = trapframe->fs;
    mcontext->gregs[JINUE_GREG_ES] = trapframe->es;
    mcontext->gregs[JINUE_GREG_DS] = trapframe->ds;
    mcontext->gregs[JINUE_GREG_EDI] = trapframe->edi;
    mcontext->gregs[JINUE_GREG_ESI] = trapframe->esi;
    mcontext->gregs[JINUE_GREG_EBP] = trapframe->ebp;
    mcontext->gregs[JINUE_GREG_ESP] = 0;
    mcontext->gregs[JINUE_GREG_EBX] = trapframe->ebx;
    mcontext->gregs[JINUE_GREG_EDX] = trapframe->edx;
    mcontext->gregs[JINUE_GREG_ECX] = trapframe->ecx;
    mcontext->gregs[JINUE_GREG_EAX] = trapframe->eax;
    mcontext->gregs[JINUE_GREG_TRAPNO] = trapframe->trapno;
    mcontext->gregs[JINUE_GREG_ERR] = trapframe->errcode;
    mcontext->gregs[JINUE_GREG_EIP] = trapframe->eip;
    mcontext->gregs[JINUE_GREG_CS] = trapframe->cs;
    mcontext->gregs[JINUE_GREG_EFL] = trapframe->eflags;
    mcontext->gregs[JINUE_GREG_UESP] = trapframe->esp;
    mcontext->gregs[JINUE_GREG_SS] = trapframe->ss;

    /* TODO store FPU state */

    mcontext->fpregs.type = JINUE_FPREGS_NONE;
    mcontext->fpregs.regs = NULL;

    siginfo->si_signo = signo;
    siginfo->si_code = 0;
    siginfo->si_errno = 0;
    siginfo->si_pid = 0;
    siginfo->si_uid = 0;
    siginfo->si_addr = NULL;
    siginfo->si_status = 0;
    siginfo->si_value.sival_ptr = NULL;
}
