#include <jinue/errno.h>
#include <jinue/pfalloc.h>
#include <hal/bootmem.h>
#include <hal/vga.h>
#include <printk.h>
#include <stddef.h>
#include <syscall.h>
#include <thread.h>


void dispatch_syscall(jinue_syscall_args_t *args) {
    switch(args->arg0) {
    
    case SYSCALL_FUNCT_SYSCALL_METHOD:
        syscall_args_set_return(args, syscall_method);
        break;
        
    case SYSCALL_FUNCT_VGA_PUTC:
        /** TODO: permission check */
        vga_putc((char)args->arg1);
        syscall_args_set_return(args, 0);
        break;
    
    case SYSCALL_FUNCT_VGA_PUTS:
        /** TODO: permission check, sanity check (data size vs buffer size) */
        vga_printn((char *)args->arg2, jinue_args_get_data_size(args));
        syscall_args_set_return(args, 0);
        break;

    case SYSCALL_FUNCT_SET_THREAD_LOCAL_ADDR:
        current_thread->local_storage      = (addr_t)args->arg1;
        current_thread->local_storage_size = (size_t)args->arg2;
        syscall_args_set_return(args, 0);
        break;

    case SYSCALL_FUNCT_GET_THREAD_LOCAL_ADDR:
        syscall_args_set_return_ptr(args, current_thread->local_storage);
        break;
    
    case SYSCALL_FUNCT_GET_FREE_MEMORY:
    {
        bootmem_t      *block;
        memory_block_t *block_dest;
        unsigned int count, count_max;

        /** TODO: check user pointer */
        size_t buffer_size  = jinue_args_get_buffer_size(args);
        block_dest          = (memory_block_t *)jinue_args_get_buffer_ptr(args);
        
        count_max = buffer_size / sizeof(memory_block_t);

        for(count = 0; count < count_max; ++count) {
            block = bootmem_get_block();
            
            if(block == NULL) {
                break;
            }
            
            block_dest->addr  = block->addr;
            block_dest->count = block->count;
            
            ++block_dest;
        }
        
        args->arg0 = (uintptr_t)count;

        if(count == count_max && bootmem_root != NULL) {
            args->arg1  = JINUE_EMORE;
        }
        else {
            args->arg1  = 0;
        }

        args->arg2 = 0;
        args->arg3 = 0;
    }
        break;
    
    default:
        printk("SYSCALL: function %u arg1=%u(0x%x) arg2=%u(0x%x) arg3=%u(0x%x)\n",
            args->arg0,
            args->arg1,   args->arg1,
            args->arg2,   args->arg2,
            args->arg3,   args->arg3 );
        
        syscall_args_set_error(args, JINUE_ENOSYS);
    }
}
