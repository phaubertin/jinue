#include <jinue/elf.h>
#include <jinue/errno.h>
#include <jinue/ipc.h>
#include <jinue/pfalloc.h>
#include <jinue/syscall.h>
#include <jinue/types.h>
#include <jinue/vm.h>
#include <printk.h>
#include <stddef.h>
#include <stdint.h>


#define MEMORY_BLOCK_MAX    32

#define THREAD_STACK_SIZE   4096

int errno;

Elf32_auxv_t *auxvp;

int fd;

char thread_a_stack[THREAD_STACK_SIZE];


void thread_a(void) {
    int errno;
    char message[] = "Hello World!";

    if(fd < 0) {
        printk("Thread A has invalid descriptor.\n");
    }
    else {
        printk("Thread A got descriptor %u.\n", fd);

        printk("Thread A is sending message: %s\n", message);

        int ret = jinue_send(
                SYSCALL_FUNCT_USER_BASE,    /* function number */
                fd,                         /* target descriptor */
                message,                    /* buffer address */
                sizeof(message),            /* buffer size */
                sizeof(message),            /* data size */
                0,                          /* number of descriptors */
                &errno);                    /* error number */

        if(ret < 0) {
            printk("jinue_send() failed with error: %u.\n", errno);
        }
        else {
            printk("Thread A got reply from main thread: %s\n", message);
        }
    }

    printk("Thread A is exiting.\n");

    jinue_thread_exit();
}

int main(int argc, char *argv[], char *envp[]) {
    memory_block_t blocks[MEMORY_BLOCK_MAX];
    int count;
    uint32_t total_memory;
    unsigned int idx;
    
    /* say hello */
    printk("Process manager (%s) started.\n", argv[0]);

    /* get system call implementation so we can use something faster than the
     * interrupt-based one if available */
    jinue_get_syscall_implementation();
    
    printk("Using system call method '%s'.\n", jinue_get_syscall_implementation_name());
    
    /* get free memory blocks from microkernel */
    errno = 0;
    count = jinue_get_free_memory(blocks, sizeof(blocks), &errno);

    if(errno == JINUE_EMORE) {
        printk("warning: could not get all memory blocks because buffer is too small.\n");
    }
    
    /* count memory */
    total_memory = 0;
    
    for(idx = 0; idx < count; ++idx) {
        total_memory += blocks[idx].count;
    }
    
    (void)jinue_yield();

    printk("%u kilobytes (%u pages) of memory available to process manager.\n", 
        (unsigned long)(total_memory * PAGE_SIZE / KB), 
        (unsigned long)(total_memory) );

    printk("Creating IPC object descriptor.\n");

    int fd = jinue_create_ipc(JINUE_IPC_NONE, &errno);

    if(fd < 0) {
        printk("Error number: %u\n", errno);
    }
    else {
        char buffer[128];
        jinue_message_t message;

        printk("Main thread got descriptor %u.\n", fd);
        
        printk("Creating thread A.\n");

        (void)jinue_thread_create(thread_a, &thread_a_stack[THREAD_STACK_SIZE], NULL);

        int ret = jinue_receive(
                fd,
                buffer,
                sizeof(buffer),
                &message,
                &errno);

        if(ret < 0) {
            printk("jinue_receive() failed with error: %u.\n", errno);
        }
        else {
            char reply[] = "OK";

            printk("Main thread received message: %s\n", buffer);

            ret = jinue_reply(
                    reply,                      /* buffer address */
                    sizeof(reply),              /* buffer size */
                    sizeof(reply),              /* data size */
                    0,                          /* number of descriptors */
                    &errno);                    /* error number */

            if(ret < 0) {
                printk("jinue_reply() failed with error: %u.\n", errno);
            }
        }
    }

    printk("Main thread is running.\n");

    while (1) {
        (void)jinue_yield();
    }
    
    return 0;
}
