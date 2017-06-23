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
    addr_space_t *proc_man_addr_space;    
    elf_info_t elf_info;
    
    /* initialize hardware abstraction layer */
    hal_init();
    
    /* create thread control block for first thread */
    current_thread = create_initial_thread(NULL);
    
    proc_man_addr_space = vm_create_addr_space();

    /* load process manager binary */
    Elf32_Ehdr *elf = find_process_manager();
    elf_load(&elf_info, elf, proc_man_addr_space);
    
    /* start process manager */
    vm_switch_addr_space(proc_man_addr_space);
    elf_start(&elf_info);
}
