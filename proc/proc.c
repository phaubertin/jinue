#include <jinue/elf.h>
#include <jinue/errno.h>
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

char thread_a_stack[THREAD_STACK_SIZE];

char thread_b_stack[THREAD_STACK_SIZE];

void thread_a(void) {
    while (1) {
        printk("Thread A is running.\n");
        (void)jinue_yield();
    }
}

void thread_b(void) {
    int counter;
    
    for(counter = 10; counter > 0; --counter) {
        printk("Thread B is running (%u).\n", counter);
        (void)jinue_yield();
    }
    
    printk("Thread B is exiting.\n");
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

    (void)jinue_thread_create(thread_a, &thread_a_stack[THREAD_STACK_SIZE], NULL);

    (void)jinue_thread_create(thread_b, &thread_b_stack[THREAD_STACK_SIZE], NULL);

    while (1) {
        printk("Main thread is running.\n");
        (void)jinue_yield();
    }
    
    return 0;
}
