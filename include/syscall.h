#ifndef JINUE_KERNEL_SYSCALL_H
#define JINUE_KERNEL_SYSCALL_H

#include <jinue-common/syscall.h>
#include <hal/syscall.h>
#include <stdint.h>

static inline void syscall_args_set_return_uintptr(jinue_syscall_args_t *args, uintptr_t retval) {
	args->arg0	= retval;
	args->arg1	= 0;
	args->arg2	= 0;
	args->arg3	= 0;
}

static inline void syscall_args_set_return(jinue_syscall_args_t *args, int retval) {
	syscall_args_set_return_uintptr(args, (uintptr_t)retval);
}

static inline void syscall_args_set_return_ptr(jinue_syscall_args_t *args, void *retval) {
	syscall_args_set_return_uintptr(args, (uintptr_t)retval);
}

static inline void syscall_args_set_error(jinue_syscall_args_t *args, uintptr_t error) {
	args->arg0	= (uintptr_t)-1;
	args->arg1	= error;
	args->arg2	= 0;
	args->arg3	= 0;
}

void dispatch_syscall(jinue_syscall_args_t *syscall_args);

#endif
