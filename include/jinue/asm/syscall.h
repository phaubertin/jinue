#ifndef _JINUE_ASM_SYSCALL_H
#define _JINUE_ASM_SYSCALL_H

/** interrupt vector for system call software interrupt */
#define SYSCALL_IRQ    0x80

/** get best system call implementation number based on CPU features */
#define SYSCALL_FUNCT_SYSCALL_METHOD             1

/** send a character to in-kernel console driver */
#define SYSCALL_FUNCT_CONSOLE_PUTC               2

/** send a fixed-length string to in-kernel console driver */
#define SYSCALL_FUNCT_CONSOLE_PUTS               3

/** create a new thread */
#define SYSCALL_FUNCT_THREAD_CREATE              4

/** relinquish the CPU and allow the next thread to run */
#define SYSCALL_FUNCT_THREAD_YIELD               5

/** set address and size of thread local storage for current thread */
#define SYSCALL_FUNCT_SET_THREAD_LOCAL_ADDR      6

/** get address of thread local storage for current thread */
#define SYSCALL_FUNCT_GET_THREAD_LOCAL_ADDR      7

/** get free memory block list for management by process manager */
#define SYSCALL_FUNCT_GET_FREE_MEMORY            8

/** create an IPC object to receive messages */
#define SYSCALL_FUNCT_CREATE_IPC                 9

/** receive a message on an IPC object */
#define SYSCALL_FUNCT_RECEIVE                   10

/** reply to current message */
#define SYSCALL_FUNCT_REPLY                     11

/** start of function numbers for process manager system calls */
#define SYSCALL_FUNCT_PROC_BASE                 0x400

/** start of function numbers for system IPC objects */
#define SYSCALL_FUNCT_SYSTEM_BASE               0x1000

/** start of function numbers for user IPC objects */
#define SYSCALL_FUNCT_USER_BASE                 0x4000


/** Intel's fast system call method (SYSENTER/SYSEXIT) */
#define SYSCALL_METHOD_FAST_INTEL       0

/** AMD's fast system call method (SYSCALL/SYSLEAVE) */
#define SYSCALL_METHOD_FAST_AMD         1

/** slow/safe system call method using interrupts */
#define SYSCALL_METHOD_INTR             2

#endif
