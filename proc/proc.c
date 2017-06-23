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


int errno;

Elf32_auxv_t *auxvp;

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
    
    printk("%u kilobytes (%u pages) of memory available to process manager.\n", 
        (unsigned long)(total_memory * PAGE_SIZE / KB), 
        (unsigned long)(total_memory) );

    /* loop forever */
    while (1) {}
    
    return 0;
}
