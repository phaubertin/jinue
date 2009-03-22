#include <alloc.h>
#include <assert.h>
#include <boot.h>
#include <kernel.h> /* includes stddef.h */
#include <panic.h>
#include <printk.h>
#include <vga.h>
#include <vm.h>

#include <vm_alloc.h>
#include <slab.h>

addr_t kernel_top;
size_t kernel_size;

void kernel(void) {
	kinit();	
	idle();
	
	panic("idle() returned.");	
}

void kinit(void) {
	boot_t *boot;
	unsigned int remainder;
	
	/* say hello */
	vga_init();	
	printk("Kernel started.\n");
	
	/* we assume the kernel starts on a page boundary */
	assert((unsigned int)kernel_start % PAGE_SIZE == 0);
	
	/* find out kernel size and set kernel_top 
	 * (top of kernel, aligned to page boundary) */
	boot = get_boot_data();
	
	kernel_size = boot->sysize * 16;
	remainder   = kernel_size % PAGE_SIZE;
	
	printk("Kernel size is %u (+%u) bytes.\n", kernel_size, PAGE_SIZE - remainder);
	
	if(remainder != 0) {
		kernel_size += PAGE_SIZE - remainder;
	}
	kernel_top  = kernel_start + kernel_size;
		
	/* initialize allocator */
	alloc_init();
}

void idle(void) {
	while(1) {}
}

