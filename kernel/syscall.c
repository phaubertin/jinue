#include <jinue/errno.h>
#include <jinue/pfalloc.h>
#include <bootmem.h>
#include <printk.h>
#include <stddef.h>
#include <syscall.h>
#include <thread.h>
#include <vga.h>


int syscall_method;


void dispatch_syscall(syscall_params_t *syscall_params) {
	bootmem_t      *block;
	memory_block_t *block_dest;
	unsigned int count;
	
	unsigned int funct;
	unsigned int arg1;
	unsigned int arg2;
	
	funct = syscall_params->args.funct;
	arg1  = syscall_params->args.arg1;
	arg2  = syscall_params->args.arg2;
	
	syscall_params->ret.errno    = 0;
	syscall_params->ret.perrno   = NULL;
	
	switch(funct) {
	
	case SYSCALL_FUNCT_SYSCALL_METHOD:
		syscall_params->ret.val = syscall_method;
		break;		
		
	case SYSCALL_FUNCT_VGA_PUTC:
		/** TODO: permission check */
		vga_putc((char)arg1);
		break;
	
	case SYSCALL_FUNCT_VGA_PUTS:
		/** TODO: permission check, sanity check */
		vga_printn((char *)arg1, arg2);
		break;

	case SYSCALL_FUNCT_SET_ERRNO_ADDR:
		current_thread->perrno = (int *)arg1;
		break;

	case SYSCALL_FUNCT_GET_ERRNO_ADDR:
		syscall_params->ret.val = (int)current_thread->perrno;
		break;

	case SYSCALL_FUNCT_SET_THREAD_LOCAL_ADDR:
		current_thread->local_storage      = (addr_t)arg1;
		current_thread->local_storage_size = (size_t)arg2;
		break;

	case SYSCALL_FUNCT_GET_THREAD_LOCAL_ADDR:
		syscall_params->ret.val = (int)current_thread->local_storage;
		break;
	
	case SYSCALL_FUNCT_GET_FREE_MEMORY:
		/** TODO: check user pointer */
		block_dest = (memory_block_t *)arg1;
		
		for(count = 0; count < arg2; ++count) {
			
			block = bootmem_get_block();
			
			if(block == NULL) {
				break;
			}
			
			block_dest->addr  = block->addr;
			block_dest->count = block->count;
			
			++block_dest;
		}
		
		syscall_params->ret.val = count;
		
		if(count == arg2) {
			if(bootmem_root != NULL) {
				syscall_params->ret.errno    = JINUE_EMORE;
				syscall_params->ret.perrno   = current_thread->perrno;
			}
		}
		
		break;
	
	default:
		syscall_params->ret.val    = -1;
		syscall_params->ret.errno  = JINUE_ENOSYS;
		syscall_params->ret.perrno = current_thread->perrno;
	}	
}
