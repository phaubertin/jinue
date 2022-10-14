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


static Elf32_Ehdr *get_kernel_elf_header(const boot_info_t *boot_info) {
    if(boot_info->kernel_start == NULL) {
        panic("malformed boot image: no kernel ELF binary");
    }

    if(boot_info->kernel_size < sizeof(Elf32_Ehdr)) {
        panic("kernel too small to be an ELF binary");
    }

    if(! elf_check(boot_info->kernel_start)) {
        panic("kernel ELF binary is invalid");
    }

    return boot_info->kernel_start;
}

static Elf32_Ehdr *get_userspace_loader_elf_header(const boot_info_t *boot_info) {
    if(boot_info->proc_start == NULL) {
        panic("malformed boot image: no user space loader ELF binary");
    }

    if(boot_info->proc_size < sizeof(Elf32_Ehdr)) {
        panic("user space loader too small to be an ELF binary");
    }

    printk("Found user space loader with size %u bytes.\n", boot_info->proc_size);

    if(! elf_check(boot_info->proc_start)) {
        panic("user space loader ELF binary is invalid");
    }

    return boot_info->proc_start;
}


void kmain(void) {
    elf_info_t elf_info;
    
    /* Retrieve the boot information structure, which contains information
     * passed to the kernel by the setup code. */
    const boot_info_t *boot_info = get_boot_info();

    /* The first thing we want to do is parse the command line options, before
     * we log anything, because some options affect logging, such as whether we
     * need to log to VGA and/or serial port, the baud rate, etc.
     *
     * We won't even validate the boot information structure yet because
     * boot_info_check() logs errors (actually panics) on failure. */
    cmdline_parse_options(boot_info->cmdline);

    /* Now that we parsed the command line options, we can initialize the
     * console (i.e. logging) properly and say hello. */
    const cmdline_opts_t *cmdline_opts = cmdline_get_options();
    console_init(cmdline_opts);

    printk("Jinue microkernel started.\n");
    printk("Kernel revision " GIT_REVISION " built " BUILD_TIME " on " BUILD_HOST "\n");
    
    printk("Kernel command line:\n", boot_info->kernel_size);
    printk("%s\n", boot_info->cmdline);
    printk("---\n");

    /* If there were issues parsing the command line, these will be reported
     * here (i.e. panic), now that the console has been initialized and we can
     * log things. */
    cmdline_report_parsing_errors();

    /* Validate the boot information structure. */
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

    /* Check and get kernel ELF header */
    Elf32_Ehdr *kernel = get_kernel_elf_header(boot_info);

    /* initialize the hardware abstraction layer */
    hal_init(kernel, cmdline_opts, &boot_alloc, boot_info);

    /* initialize caches */
    ipc_boot_init();
    process_boot_init();

    /* create process for process manager */
    process_t *process = process_create();

    if(process == NULL) {
        panic("Could not create initial process.");
    }

    process_switch_to(process);

    /* load user space loader binary */
    Elf32_Ehdr *loader = get_userspace_loader_elf_header(boot_info);

    elf_load(
            &elf_info,
            loader,
            "jinue-userspace-loader",
            boot_info->cmdline,
            &process->addr_space,
            &boot_alloc);

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

    /* Start first thread */
    thread_start_first();

    /* should never happen */
    panic("thread_start_first() returned in kmain()");
}
