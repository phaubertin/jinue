/*
 * Copyright (C) 2019 Philippe Aubertin.
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the author nor the names of other contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <hal/boot.h>
#include <hal/hal.h>
#include <hal/vm.h>
#include <boot.h>
#include <console.h>
#include <elf.h>
#include <hal/memory.h>
#include <ipc.h>
#include <kmain.h>
#include <panic.h>
#include <printk.h>
#include <process.h>
#include <stddef.h>
#include <syscall.h>
#include <thread.h>
#include "build-info.gen.h"


static Elf32_Ehdr *find_process_manager(const boot_info_t *boot_info) {
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
    
    /* Say hello. */
    printk("Kernel revision " GIT_REVISION " built " BUILD_TIME " on " BUILD_HOST "\n");
    
    const boot_info_t *boot_info = get_boot_info();
    (void)boot_info_check(true);

    if(boot_info->ramdisk_start == 0 || boot_info->ramdisk_size == 0) {
        printk("%kWarning: no initial RAM disk loaded.\n", VGA_COLOR_YELLOW);
    }
    else {
        printk("RAM disk with size %u bytes loaded at address %x.\n", boot_info->ramdisk_size, boot_info->ramdisk_start);
    }

    printk("Kernel command line:\n", boot_info->kernel_size);
    printk("    %s\n", boot_info->cmdline);
    printk("---\n");

    /* Initialize the boot allocator. */
    boot_alloc_t boot_alloc;
    boot_alloc_init(&boot_alloc, boot_info);

    /* initialize hardware abstraction layer */
    hal_init(&boot_alloc, boot_info);

    /* initialize caches */
    ipc_boot_init();
    process_boot_init();

    /* create process for process manager */
    process_t *process = process_create();

    if(process == NULL) {
        panic("Could not create initial process.");
    }

    process_switch_to(process);

    /* load process manager binary */
    Elf32_Ehdr *elf = find_process_manager(boot_info);

    elf_load(&elf_info, elf, &process->addr_space, &boot_alloc);

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
