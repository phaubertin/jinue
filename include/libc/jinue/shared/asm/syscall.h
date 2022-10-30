/*
 * Copyright (C) 2019 Philippe Aubertin.
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

#ifndef _JINUE_SHARED_ASM_SYSCALL_H
#define _JINUE_SHARED_ASM_SYSCALL_H

/* TODO definitions in this file need a JINUE_ prefix */

/** interrupt vector for system call software interrupt */
#define SYSCALL_IRQ                     0x80

/** send a fixed-length string to in-kernel console driver */
#define SYSCALL_FUNC_PUTS               3

/** create a new thread */
#define SYSCALL_FUNC_CREATE_THREAD      4

/** relinquish the CPU and allow the next thread to run */
#define SYSCALL_FUNC_YIELD_THREAD       5

/** set address and size of thread local storage for current thread */
#define SYSCALL_FUNC_SET_THREAD_LOCAL   6

/** get address of thread local storage for current thread */
#define SYSCALL_FUNC_GET_THREAD_LOCAL   7

/** get free memory block list for management by user space */
#define SYSCALL_FUNC_GET_USER_MEMORY    8

/** create an IPC object to receive messages */
#define SYSCALL_FUNC_CREATE_IPC         9

/** receive a message on an IPC object */
#define SYSCALL_FUNC_RECEIVE            10

/** reply to current message */
#define SYSCALL_FUNC_REPLY              11

/** destroy the current thread */
#define SYSCALL_FUNC_EXIT_THREAD        12


/** start of function numbers for user space messages */
#define SYSCALL_USER_BASE               4096

/** slow/safe interrupt-based system call implementation */
#define SYSCALL_IMPL_INTR               0

/** AMD's fast system call implementation (SYSCALL/SYSLEAVE) */
#define SYSCALL_IMPL_FAST_AMD           1

/** Intel's fast system call implementation (SYSENTER/SYSEXIT) */
#define SYSCALL_IMPL_FAST_INTEL         2

/** last system call implementation index */
#define SYSCALL_IMPL_LAST               2

#endif
