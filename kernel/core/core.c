#include <hal/bootmem.h>
#include <hal/hal.h>
#include <hal/vm.h>
#include <core.h>
#include <elf.h>
#include <panic.h>
#include <printk.h>
#include <syscall.h>
#include <thread.h>
#include "build-info.gen.h"


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
    
    printk("Kernel build " GIT_REVISION " " BUILD_TIME "\n");

    /* load process manager binary */
    Elf32_Ehdr *elf = find_process_manager();
    elf_load(&elf_info, elf, &initial_addr_space);

    /* create initial thread context */
    thread_t *thread = thread_create(
            &initial_addr_space,
            elf_info.entry,
            elf_info.stack_addr);
    
    if(thread == NULL) {
        panic("Could not create initial thread.");
    }
    
    /* start process manager
     *
     * We switch from NULL since this is the first thread. */
    thread_yield_from(NULL, false);

    /* should never happen */
    panic("thread_yield_from() returned in kmain()");
}
