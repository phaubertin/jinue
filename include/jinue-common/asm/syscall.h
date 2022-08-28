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

#ifndef _JINUE_COMMON_ASM_SYSCALL_H
#define _JINUE_COMMON_ASM_SYSCALL_H

/** interrupt vector for system call software interrupt */
#define SYSCALL_IRQ    0x80

/** get best system call implementation number based on CPU features */
#define SYSCALL_FUNC_GET_SYSCALL_METHOD          1

/** send a character to in-kernel console driver */
#define SYSCALL_FUNC_CONSOLE_PUTC                2

/** send a fixed-length string to in-kernel console driver */
#define SYSCALL_FUNC_CONSOLE_PUTS                3

/** create a new thread */
#define SYSCALL_FUNC_THREAD_CREATE               4

/** relinquish the CPU and allow the next thread to run */
#define SYSCALL_FUNC_THREAD_YIELD                5

/** set address and size of thread local storage for current thread */
#define SYSCALL_FUNC_SET_THREAD_LOCAL_ADDR       6

/** get address of thread local storage for current thread */
#define SYSCALL_FUNC_GET_THREAD_LOCAL_ADDR       7

/** get free memory block list for management by user space */
#define SYSCALL_FUNC_GET_USER_MEMORY             8

/** create an IPC object to receive messages */
#define SYSCALL_FUNC_CREATE_IPC_ENDPOINT         9

/** receive a message on an IPC object */
#define SYSCALL_FUNC_RECEIVE                    10

/** reply to current message */
#define SYSCALL_FUNC_REPLY                      11


/** start of function numbers for user space messages */
#define SYSCALL_USER_BASE                       0x1000


/** Intel's fast system call method (SYSENTER/SYSEXIT) */
#define SYSCALL_METHOD_FAST_INTEL       0

/** AMD's fast system call method (SYSCALL/SYSLEAVE) */
#define SYSCALL_METHOD_FAST_AMD         1

/** slow/safe system call method using interrupts */
#define SYSCALL_METHOD_INTR             2

#endif
