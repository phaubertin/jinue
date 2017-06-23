#ifndef _JINUE_KERNEL_HAL_SYSCALL_H_
#define _JINUE_KERNEL_HAL_SYSCALL_H_

#include <jinue/syscall.h>


extern int syscall_method;

/** entry point for Intel fast system call mechanism (SYSENTER/SYSEXIT) */
void fast_intel_entry(void);

/** entry point for AMD fast system call mechanism (SYSCALL/SYSRET) */
void fast_amd_entry(void);

void dispatch_syscall(jinue_syscall_args_t *args);

#endif
