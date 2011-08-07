#ifndef _JINUE_KERNEL_IPC_H_
#define _JINUE_KERNEL_IPC_H_

#include <jinue/ipc.h>

typedef struct {
	ipc_ref_t    dest;
	unsigned int funct;
	unsigned int arg1;
	unsigned int arg2;	
} ipc_params_t;


void dispatch_ipc(ipc_ref_t dest, unsigned int funct, unsigned int arg1, unsigned int arg2);

void dispatch_syscall(unsigned int funct, unsigned int arg1, unsigned int arg2);

#endif
