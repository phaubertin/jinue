#ifndef _JINUE_KERNEL_SYSCALL_H_
#define _JINUE_KERNEL_SYSCALL_H_

#include <jinue/syscall.h>

extern int syscall_method;

void dispatch_syscall(ipc_params_t *ipc_params);

#endif
