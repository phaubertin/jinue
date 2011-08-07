#ifndef _PROC_SYSCALL_H_
#define _PROC_SYSCALL_H_

#include <jinue/ipc.h>

int syscall_intr(ipc_ref_t dest, unsigned int funct, unsigned int arg1, unsigned int arg2);

#endif
