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
#include <hal/vga.h>
#include <hal/vm.h>
#include <boot.h>
#include <cmdline.h>
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
    
    const boot_info_t *boot_info = get_boot_info();

    /* TODO reword this
     *
     * The boot_info structure has not been validated yet, so let's not take any
     * chances. We want to parse the command line before doing anything that
     * logs to the console (including anything that can fail like validating the
     * boot_info structure) because the command line might contain arguments
     * that control where we log (VGA and/or UART) as well as other relevant
     * settings (e.g. UART baud rate). */
    cmdline_parse_options(boot_info->cmdline);

    const cmdline_opts_t *cmdline_opts = cmdline_get_options();

    /* initialize console and say hello */
    console_init(cmdline_opts);

    printk("Jinue microkernel started.\n");
    printk("Kernel revision " GIT_REVISION " built " BUILD_TIME " on " BUILD_HOST "\n");
    
    printk("Kernel command line:\n", boot_info->kernel_size);
    printk("%s\n", boot_info->cmdline);
    printk("---\n");

    cmdline_process_errors();

    (void)boot_info_check(true);

    if(boot_info->ramdisk_start == 0 || boot_info->ramdisk_size == 0) {
        /* TODO once user loader is implemented, this needs to be a kernel panic. */
        printk("%kWarning: no initial RAM disk loaded.\n", VGA_COLOR_YELLOW);
    }
    else {
        printk("Bootloader has loaded RAM disk with size %u bytes at address %x.\n", boot_info->ramdisk_size, boot_info->ramdisk_start);
    }

    /* Initialize the boot allocator. */
    boot_alloc_t boot_alloc;
    boot_alloc_init(&boot_alloc, boot_info);

    /* initialize hardware abstraction layer */
    hal_init(&boot_alloc, boot_info, cmdline_opts);

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

    /* This should be the last thing the kernel prints before passing control
     * to the user space loader. */
    printk("---\n");

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
