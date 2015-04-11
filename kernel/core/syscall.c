#include <jinue/errno.h>
#include <jinue/pfalloc.h>
#include <hal/bootmem.h>
#include <hal/vga.h>
#include <printk.h>
#include <stddef.h>
#include <syscall.h>
#include <thread.h>


void dispatch_syscall(syscall_params_t *syscall_params) {
    bootmem_t      *block;
    memory_block_t *block_dest;
    unsigned int count;
    int  retval;
    int  errno;
    
    retval   = 0;
    errno    = 0;
    
    switch(syscall_params->args.funct) {
    
    case SYSCALL_FUNCT_SYSCALL_METHOD:
        retval = syscall_method;
        break;        
        
    case SYSCALL_FUNCT_VGA_PUTC:
        /** TODO: permission check */
        vga_putc((char)syscall_params->args.arg1);
        break;
    
    case SYSCALL_FUNCT_VGA_PUTS:
        /** TODO: permission check, sanity check */
        vga_printn((char *)syscall_params->args.arg1, syscall_params->args.arg2);
        break;

    case SYSCALL_FUNCT_SET_ERRNO_ADDR:
        current_thread->perrno = (int *)syscall_params->args.arg1;
        break;

    case SYSCALL_FUNCT_GET_ERRNO_ADDR:
        retval = (int)current_thread->perrno;
        break;

    case SYSCALL_FUNCT_SET_THREAD_LOCAL_ADDR:
        current_thread->local_storage      = (addr_t)syscall_params->args.arg1;
        current_thread->local_storage_size = (size_t)syscall_params->args.arg2;
        break;

    case SYSCALL_FUNCT_GET_THREAD_LOCAL_ADDR:
        retval = (int)current_thread->local_storage;
        break;
    
    case SYSCALL_FUNCT_GET_FREE_MEMORY:
        /** TODO: check user pointer */
        block_dest = (memory_block_t *)syscall_params->args.arg1;
        
        for(count = 0; count < syscall_params->args.arg2; ++count) {
            
            block = bootmem_get_block();
            
            if(block == NULL) {
                break;
            }
            
            block_dest->addr  = block->addr;
            block_dest->count = block->count;
            
            ++block_dest;
        }
        
        retval = count;
        
        if(count == syscall_params->args.arg2) {
            if(bootmem_root != NULL) {
                errno    = JINUE_EMORE;
            }
        }
        
        break;
    
    default:
        printk("SYSCALL: ref 0x%x funct %u: arg1=%u(0x%x) arg2=%u(0x%x) method=%u(0x%x) \n",
            syscall_params->args.dest,
            syscall_params->args.funct,
            syscall_params->args.arg1,   syscall_params->args.arg1,
            syscall_params->args.arg2,   syscall_params->args.arg2,
            syscall_params->args.method, syscall_params->args.method );
        
        retval = -1;
        errno  = JINUE_ENOSYS;
    }
    
    syscall_params->ret.retval  = retval;
    syscall_params->ret.errno   = errno;
    
    if(errno != 0) {
        syscall_params->ret.perrno = current_thread->perrno;
    }
    else {
        syscall_params->ret.perrno = NULL;
    }
}
