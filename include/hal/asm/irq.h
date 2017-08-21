/*
 * Copyright (C) 2017 Philippe Aubertin.
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

#ifndef JINUE_HAL_ASM_IRQ_H
#define JINUE_HAL_ASM_IRQ_H

#define IDT_VECTOR_COUNT        256

#define IDT_FIRST_IRQ           32

#define IDT_IRQ_COUNT           (IDT_VECTOR_COUNT - IDT_FIRST_IRQ)


/** Divide Error */
#define EXCEPTION_DIV_ZERO               0

/** NMI Interrupt */
#define EXCEPTION_NMI                    2

/** Breakpoint */
#define EXCEPTION_BREAK                  3

/** Overflow */
#define EXCEPTION_OVERFLOW               4

/** BOUND Range Exceeded */
#define EXCEPTION_BOUND                  5

/** Invalid Opcode (Undefined Opcode) */
#define EXCEPTION_INVALID_OP             6

/** Device Not Available (No Math Coprocessor) */
#define EXCEPTION_NO_COPROC              7

/** Double Fault */
#define EXCEPTION_DOUBLE_FAULT           8

/** Invalid TSS */
#define EXCEPTION_INVALID_TSS           10

/** Segment Not Present */
#define EXCEPTION_SEGMENT_NOT_PRESENT   11

/** Stack-Segment Fault */
#define EXCEPTION_STACK_SEGMENT         12

/** General Protection */
#define EXCEPTION_GENERAL_PROTECTION    13

/** Page Fault */
#define EXCEPTION_PAGE_FAULT            14

/** x87 FPU Floating-Point Error (Math Fault) */
#define EXCEPTION_MATH                  16

/** Alignment Check */
#define EXCEPTION_ALIGNMENT             17

/** Machine Check */
#define EXCEPTION_MACHINE_CHECK         18

/** SIMD Floating-Point Exception */
#define EXCEPTION_SIMD                  19


#endif

