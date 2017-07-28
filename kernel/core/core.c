#include <hal/boot.h>
#include <hal/bootmem.h>
#include <hal/hal.h>
#include <hal/vm.h>
#include <core.h>
#include <console.h>
#include <elf.h>
#include <ipc.h>
#include <panic.h>
#include <printk.h>
#include <process.h>
#include <stddef.h>
#include <syscall.h>
#include <thread.h>
#include "build-info.gen.h"


static Elf32_Ehdr *find_process_manager(void) {
    const boot_info_t *boot_info = get_boot_info();

    if(boot_info->proc_start == NULL) {
        panic("Malformed boot image");
    }

    if(boot_info->proc_size < sizeof(Elf32_Ehdr)) {
        panic("Too small to be an ELF binary");
    }

    printk("Found process manager binary with size %u bytes.\n", boot_info->proc_size);

    return boot_info->proc_start;
}


void kmain(void) {
    elf_info_t elf_info;
    
    /* initialize console and say hello */
    console_init();
    
    printk("Kernel build " GIT_REVISION " " BUILD_TIME "\n");
    
    /* initialize hardware abstraction layer */
    hal_init();

    /* initialize caches */
    ipc_boot_init();
    process_boot_init();

    /* create process for process manager */
    process_t *process = process_create();

    /* load process manager binary */
    Elf32_Ehdr *elf = find_process_manager();
    elf_load(&elf_info, elf, &process->addr_space);

    /* create initial thread */
    thread_t *thread = thread_create(
            process,
            elf_info.entry,
            elf_info.stack_addr);
    
    if(thread == NULL) {
        panic("Could not create initial thread.");
    }
    
    /* start process manager
     *
     * We switch from NULL since this is the first thread. */
    thread_yield_from(
            NULL,
            false,      /* don't block */
            false);     /* don't destroy */
                        /* just be nice */

    /* should never happen */
    panic("thread_yield_from() returned in kmain()");
}
