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

/** interrupt vector for system call software interrupt */
#define JINUE_SYSCALL_IRQ               0x80


/** rebooot the system */
#define JINUE_SYS_REBOOT                2

/** send a fixed-length string to kernel logger */
#define JINUE_SYS_PUTS                  3

/** create a new thread */
#define JINUE_SYS_CREATE_THREAD         4

/** relinquish the CPU and allow the next thread to run */
#define JINUE_SYS_YIELD_THREAD          5

/** set address and size of thread local storage for current thread */
#define JINUE_SYS_SET_THREAD_LOCAL      6

/** get address of thread local storage for current thread */
#define JINUE_SYS_GET_THREAD_LOCAL      7

/** get free memory block list for management by user space */
#define JINUE_SYS_GET_USER_MEMORY       8

/** create an IPC object to receive messages */
#define JINUE_SYS_CREATE_IPC            9

/** receive a message on an IPC object */
#define JINUE_SYS_RECEIVE               10

/** reply to current message */
#define JINUE_SYS_REPLY                 11

/** destroy the current thread */
#define JINUE_SYS_EXIT_THREAD           12

/** map memory into the address space of a process */
#define JINUE_SYS_MMAP                  13

/** get descriptor that represents current process */
#define JINUE_SYS_GET_PROCESS           14

/** start of function numbers for user space messages */
#define JINUE_SYS_USER_BASE             4096


/** slow/safe interrupt-based system call implementation */
#define JINUE_SYSCALL_IMPL_INTERRUPT    0

/** AMD's fast system call implementation (SYSCALL/SYSLEAVE) */
#define JINUE_SYSCALL_IMPL_FAST_AMD     1

/** Intel's fast system call implementation (SYSENTER/SYSEXIT) */
#define JINUE_SYSCALL_IMPL_FAST_INTEL   2

/** last system call implementation index */
#define JINUE_SYSCALL_IMPL_LAST         2


/** maximum string length for the PUTS system call */
#define JINUE_PUTS_MAX_LENGTH           120

/** log level "info" for the PUTS system call */
#define JINUE_PUTS_LOGLEVEL_INFO        'I'

/** log level "warning" for the PUTS system call */
#define JINUE_PUTS_LOGLEVEL_WARNING     'W'

/** log level "error" for the PUTS system call */
#define JINUE_PUTS_LOGLEVEL_ERROR       'E'


/** map page with no access permission */
#define JINUE_PROT_NONE         0

/** map page with read permission */
#define JINUE_PROT_READ         (1<<0)

/** map page with write permission */
#define JINUE_PROT_WRITE        (1<<1)

/** map page with execution permission */
#define JINUE_PROT_EXEC         (1<<2)

#endif
