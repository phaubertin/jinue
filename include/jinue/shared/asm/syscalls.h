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

#ifndef _JINUE_SHARED_ASM_SYSCALLS_H
#define _JINUE_SHARED_ASM_SYSCALLS_H

/** reboot the system */
#define JINUE_SYS_REBOOT                2

/** send a fixed-length string to kernel logger */
#define JINUE_SYS_PUTS                  3

/** create a new thread */
#define JINUE_SYS_CREATE_THREAD         4

/** relinquish the CPU and allow the next thread to run */
#define JINUE_SYS_YIELD_THREAD          5

/** set address and size of thread local storage for current thread */
#define JINUE_SYS_SET_THREAD_LOCAL      6

/** get system and kernel address map */
#define JINUE_SYS_GET_ADDRESS_MAP       8

/** create an IPC object to receive messages */
#define JINUE_SYS_CREATE_ENDPOINT       9

/** receive a message on an IPC object */
#define JINUE_SYS_RECEIVE               10

/** reply to current message */
#define JINUE_SYS_REPLY                 11

/** destroy the current thread */
#define JINUE_SYS_EXIT_THREAD           12

/** map memory into the address space of a process */
#define JINUE_SYS_MMAP                  13

/** create a new process */
#define JINUE_SYS_CREATE_PROCESS        14

/** duplicate a descriptor from the current process to another */
#define JINUE_SYS_DUP                   16

/** close a descriptor in the current process */
#define JINUE_SYS_CLOSE                 17

/** destroy a kernel object through a descriptor in the current process */
#define JINUE_SYS_DESTROY               18

/** create a descriptor with specified cookie and permissions */
#define JINUE_SYS_MINT                  19

/** start a thread */
#define JINUE_SYS_START_THREAD          20

/** wait for a thread to exit */
#define JINUE_SYS_AWAIT_THREAD          21

/** reply to current message with an error code */
#define JINUE_SYS_REPLY_ERROR           22

/** start of function numbers for user space messages */
#define JINUE_SYS_USER_BASE             4096

#endif
