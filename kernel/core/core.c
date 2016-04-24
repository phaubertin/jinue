#include <hal/bootmem.h>
#include <hal/hal.h>
#include <hal/vm.h>
#include <core.h>
#include <elf.h>
#include <panic.h>
#include <printk.h>
#include <syscall.h>
#include <thread.h>

static Elf32_Ehdr *find_process_manager(void) {
	if((uintptr_t)&proc_elf_end < (uintptr_t)&proc_elf) {
		panic("Malformed boot image");
	}

	if((uintptr_t)&proc_elf_end - (uintptr_t)&proc_elf < sizeof(Elf32_Ehdr)) {
		panic("Too small to be an ELF binary");
	}

	printk("Found process manager binary with size %u bytes.\n", (unsigned int)&proc_elf_end - (unsigned int)&proc_elf);

	return &proc_elf;
}


void kmain(void) {
    elf_info_t elf_info;
    
    /* initialize hardware abstraction layer */
    hal_init();
    
    /* create thread control block for first thread */
    current_thread = (thread_t *)boot_heap;
    boot_heap = (thread_t *)boot_heap + 1;
    thread_init(current_thread, NULL);
    
    /* load process manager binary */
    Elf32_Ehdr *elf = find_process_manager();
    elf_load(&elf_info, elf, &initial_addr_space);
    
    /* start process manager */
    elf_start(&elf_info);
}
