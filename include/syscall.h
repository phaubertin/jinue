#ifndef _JINUE_KERNEL_SYSCALL_H_
#define _JINUE_KERNEL_SYSCALL_H_

#include <jinue/syscall.h>

extern int syscall_method;

void dispatch_syscall(ipc_params_t *ipc_params);

/** entry point for Intel fast sytem call mechanism (sysenter/sysexit) */
void fast_intel_entry(void);

/** entry point for AMD fast sytem call mechanism (syscall/sysret) */
void fast_amd_entry(void);

#endif
