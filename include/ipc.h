#ifndef _JINUE_KERNEL_IPC_H_
#define _JINUE_KERNEL_IPC_H_

#include <jinue/ipc.h>
#include <jinue/types.h>

typedef union {
	struct {
		ipc_ref_t    dest;
		unsigned int method;
		unsigned int funct;
		unsigned int arg1;
		unsigned int arg2;
	} args;
	struct {
		int  errno;
		int  reserved1;
		int  val;
		int  reserved2;
		int *perrno;
	} ret;
	struct {
		unsigned long ebx;
		unsigned long edx;
		unsigned long eax;
		unsigned long esi;
		unsigned long edi;		
	} regs;
} ipc_params_t;


void dispatch_ipc(ipc_params_t *ipc_params);

#endif
