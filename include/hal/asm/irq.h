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

