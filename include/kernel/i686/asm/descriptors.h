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

#ifndef JINUE_KERNEL_I686_ASM_DESCRIPTORS_H
#define JINUE_KERNEL_I686_ASM_DESCRIPTORS_H

#define SEG_SELECTOR(index, rpl) \
    ( ((index) << 3) | ((rpl) & 0x3) )

#define RPL_KERNEL               0

#define RPL_USER                 3

/** GDT entry for the null descriptor */
#define GDT_NULL                 0

/** GDT entry for kernel code segment */
#define GDT_KERNEL_CODE          1

/** GDT entry for kernel data segment */
#define GDT_KERNEL_DATA          2

/** GDT entry for user code segment */
#define GDT_USER_CODE            3

/** GDT entry for user data segment */
#define GDT_USER_DATA            4

/** GDT entry for task-state segment (TSS) */
#define GDT_TSS                  5

/** GDT entry for per-cpu data (includes the TSS) */
#define GDT_PER_CPU_DATA         6

/** GDT entry for thread-local storage */
#define GDT_USER_TLS_DATA        7

/** number of descriptors in GDT */
#define GDT_LENGTH               8

/** offset of descriptor type in descriptor */
#define SEG_FLAGS_OFFSET            40

/** size of the task-state segment (TSS) */
#define TSS_LIMIT                   104

/** segment is present */
#define SEG_FLAG_PRESENT            (1<<7)

/** system segment (i.e. call-gate, etc.) */
#define SEG_FLAG_SYSTEM             0

/** code/data/stack segment */
#define SEG_FLAG_NOSYSTEM           (1<<4)

/** 32-bit segment */
#define SEG_FLAG_32BIT              (1<<14)

/** 16-bit segment */
#define SEG_FLAG_16BIT              0

/** 32-bit gate */
#define SEG_FLAG_32BIT_GATE         (1<<3)

/** 16-bit gate */
#define SEG_FLAG_16BIT_GATE         0

/** task is busy (for TSS descriptor) */
#define SEG_FLAG_BUSY               (1<<1)

/** limit has page granularity */
#define SEG_FLAG_IN_PAGES           (1<<15)

/** limit has byte granularity */
#define SEG_FLAG_IN_BYTES           0

/** kernel/supervisor segment (privilege level 0) */
#define SEG_FLAG_KERNEL             0

/** user segment (privilege level 3) */
#define SEG_FLAG_USER               (3<<5)

/** commonly used segment flags */
#define SEG_FLAG_NORMAL \
    (SEG_FLAG_32BIT | SEG_FLAG_IN_PAGES | SEG_FLAG_NOSYSTEM | SEG_FLAG_PRESENT)

/** commonly used gate flags */
#define SEG_FLAG_NORMAL_GATE \
    (SEG_FLAG_32BIT_GATE | SEG_FLAG_SYSTEM | SEG_FLAG_PRESENT)

/** commonly used  flags for task-state segment */
#define SEG_FLAG_TSS \
    (SEG_FLAG_IN_BYTES | SEG_FLAG_SYSTEM | SEG_FLAG_PRESENT)


/** read-only data segment */
#define SEG_TYPE_READ_ONLY           0

/** read/write data segment */
#define SEG_TYPE_DATA                2

/** task gate */
#define SEG_TYPE_TASK_GATE           5

/** interrupt gate */
#define SEG_TYPE_INTERRUPT_GATE      6

/** trap gate */
#define SEG_TYPE_TRAP_GATE           7

/** task-state segment (TSS) */
#define SEG_TYPE_TSS                 9

/** code segment */
#define SEG_TYPE_CODE               10

/** call gate */
#define SEG_TYPE_CALL_GATE          12


#endif
