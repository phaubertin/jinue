#ifndef _JINUE_KERNEL_SYSCALL_H_
#define _JINUE_KERNEL_SYSCALL_H_


#define SYSCALL_METHOD_FAST_INTEL	0

#define SYSCALL_METHOD_FAST_AMD		1

#define SYSCALL_METHOD_INTR			2


int syscall_fast_intel(ipc_ref_t dest, unsigned int funct, unsigned int arg1, unsigned int arg2);

int syscall_fast_amd(ipc_ref_t dest, unsigned int funct, unsigned int arg1, unsigned int arg2);

int syscall_intr(ipc_ref_t dest, unsigned int funct, unsigned int arg1, unsigned int arg2);

#endif
