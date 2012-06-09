#ifndef _JINUE_KERNEL_SYSCALL_H_
#define _JINUE_KERNEL_SYSCALL_H_

#include <jinue/syscall.h>
#include <hal/syscall.h>


void dispatch_syscall(syscall_params_t *syscall_params);


#endif
