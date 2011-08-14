#ifndef _JINUE_SYSCALL_H_
#define _JINUE_SYSCALL_H_

#include <jinue/ipc.h>

/** interrupt vector for system call software interrupt */
#define SYSCALL_IRQ	0x80

/** get best system call method number based on CPU features */
#define SYSCALL_FUNCT_SYSCALL_METHOD			1

/** send a character to in-kernel vga driver for printing on screen */
#define SYSCALL_FUNCT_VGA_PUTC					2

/** send a fixed-length string to in-kernel vga driver for printing on screen */
#define SYSCALL_FUNCT_VGA_PUTS					3

/** set address of errno for current thread */
#define SYSCALL_FUNCT_SET_ERRNO_ADDR			4

/** get address of errno for current thread */
#define SYSCALL_FUNCT_GET_ERRNO_ADDR			5

/** set address and size of thread local storage for current thread */
#define SYSCALL_FUNCT_SET_THREAD_LOCAL_ADDR		6

/** get address of thread local storage for current thread */
#define SYSCALL_FUNCT_GET_THREAD_LOCAL_ADDR		7

/** get free memory block list for management by process manager */
#define SYSCALL_FUNCT_GET_FREE_MEMORY			8


/** Intel's fast system call method (SYSENTER/SYSEXIT) */
#define SYSCALL_METHOD_FAST_INTEL	0

/** AMD's fast system call method (SYSCALL/SYSLEAVE) */
#define SYSCALL_METHOD_FAST_AMD		1

/** slow/safe system call method using interrupts */
#define SYSCALL_METHOD_INTR			2


typedef int (*syscall_stub_t)(ipc_ref_t dest, unsigned int method, unsigned int funct, unsigned int arg1, unsigned int arg2);

extern syscall_stub_t syscall;

extern syscall_stub_t syscall_stubs[];

extern char *syscall_stub_names[];


int syscall_fast_intel(ipc_ref_t dest, unsigned int method, unsigned int funct, unsigned int arg1, unsigned int arg2);

int syscall_fast_amd(ipc_ref_t dest, unsigned int method, unsigned int funct, unsigned int arg1, unsigned int arg2);

int syscall_intr(ipc_ref_t dest, unsigned int method, unsigned int funct, unsigned int arg1, unsigned int arg2);


int get_syscall_method(void);

int set_syscall_method(void);

void set_errno_addr(int *perrno);

#endif
