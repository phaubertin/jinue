#include <jinue/console.h>
#include <jinue/errno.h>
#include <jinue/pfalloc.h>
#include <hal/bootmem.h>
#include <hal/cpu_data.h>
#include <hal/thread.h>
#include <printk.h>
#include <stddef.h>
#include <syscall.h>
#include <thread.h>


void dispatch_syscall(jinue_syscall_args_t *args) {
    switch(args->arg0) {
    
    case SYSCALL_FUNCT_SYSCALL_METHOD:
        syscall_args_set_return(args, syscall_method);
        break;
        
    case SYSCALL_FUNCT_CONSOLE_PUTC:
        /** TODO: permission check */
        console_putc((char)args->arg1);
        syscall_args_set_return(args, 0);
        break;
    
    case SYSCALL_FUNCT_CONSOLE_PUTS:
        /** TODO: permission check, sanity check (data size vs buffer size) */
        console_printn((char *)args->arg2, jinue_args_get_data_size(args));
        syscall_args_set_return(args, 0);
        break;
    
    case SYSCALL_FUNCT_THREAD_CREATE:
    {
        thread_t *thread = thread_create(
                /* TODO use arg1 as an address space reference if specified */
                get_current_addr_space(),
                (addr_t)args->arg2,
                (addr_t)args->arg3);

        if(thread == NULL) {
            syscall_args_set_error(args, JINUE_EAGAIN);
        }
        else {
            syscall_args_set_return(args, 0);
        }
    }
        break;
    
    case SYSCALL_FUNCT_THREAD_YIELD:
        thread_yield_from(get_current_thread(), args->arg1);
        syscall_args_set_return(args, 0);
        break;

    case SYSCALL_FUNCT_SET_THREAD_LOCAL_ADDR:
        thread_context_set_local_storage(
                &get_current_thread()->thread_ctx,
                (addr_t)args->arg1,
                (size_t)args->arg2);
        syscall_args_set_return(args, 0);
        break;

    case SYSCALL_FUNCT_GET_THREAD_LOCAL_ADDR:
        syscall_args_set_return_ptr(
                args,
                thread_context_get_local_storage(
                        &get_current_thread()->thread_ctx));
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
