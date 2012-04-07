#include <jinue/alloc.h>
#include <jinue/errno.h>
#include <jinue/syscall.h>
#include <jinue/types.h>
#include <jinue/vm.h>
#include <printk.h>
#include <stddef.h>

#define MEMORY_BLOCK_MAX	32


int errno;


int main(void) {
	memory_block_t blocks[MEMORY_BLOCK_MAX];
	int method;
	int count;
	uint32_t total_memory;
	unsigned int idx;
	
	/* set system call method and say hello */
	method = set_syscall_method();
	
	printk("Process manager started.\n");	
	printk("Using system call method '%s'.\n", syscall_stub_names[method]);	
	
	/* get free memory blocks from microkernel */
	errno = 0;	
	count = syscall(SYSCALL_IPC_REF, NULL, SYSCALL_FUNCT_GET_FREE_MEMORY, (unsigned int)blocks, MEMORY_BLOCK_MAX);
	
	if(errno == JINUE_EMORE) {
		printk("warning: could not get all memory blocks because buffer is too small.\n");
	}
	
	/* count memory */
	total_memory = 0;
	
	for(idx = 0; idx < count; ++idx) {
		total_memory += blocks[idx].count;
	}
	
	printk("%u kilobytes (%u pages) of memory available to process manager.\n", 
		(unsigned long)(total_memory * PAGE_SIZE / 1024), 
		(unsigned long)(total_memory) );	

	/* loop forever */
	while (1) {}
	
	return 0;
}
