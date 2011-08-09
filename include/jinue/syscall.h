#ifndef _JINUE_SYSCALL_H_
#define _JINUE_SYSCALL_H_

#include <jinue/ipc.h>

/** interrupt vector for system call software interrupt */
#define SYSCALL_IRQ	0x80

#define SYSCALL_FUNCT_SYSCALL_METHOD	0

#define SYSCALL_FUNCT_VGA_PUTC			1

#define SYSCALL_FUNCT_VGA_PUTS			2


#define SYSCALL_METHOD_FAST_INTEL	0

#define SYSCALL_METHOD_FAST_AMD		1

#define SYSCALL_METHOD_INTR			2


typedef int (*syscall_stub_t)(ipc_ref_t dest, unsigned int funct, unsigned int arg1, unsigned int arg2);

extern syscall_stub_t syscall;

extern syscall_stub_t syscall_stubs[];


int syscall_fast_intel(ipc_ref_t dest, unsigned int funct, unsigned int arg1, unsigned int arg2);

int syscall_fast_amd(ipc_ref_t dest, unsigned int funct, unsigned int arg1, unsigned int arg2);

int syscall_intr(ipc_ref_t dest, unsigned int funct, unsigned int arg1, unsigned int arg2);

int get_syscall_method(void);

void set_syscall_method(void);

#endif
