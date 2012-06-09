#include <hal/bootmem.h>
#include <elf.h>
#include <syscall.h>
#include <thread.h>


void main(void) {
    /* create thread control block for first thread */
    current_thread = (thread_t *)boot_heap;
    boot_heap = (thread_t *)boot_heap + 1;
    thread_init(current_thread, NULL);
    
    /* set system call dispatch function */
	set_syscall_funct( dispatch_syscall );

	/* load process manager binary */
    elf_load_process_manager();
    
    /* start process manager */
    elf_start_process_manager();
}
