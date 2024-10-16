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

#include <kernel/machine/init.h>
#include <kernel/cmdline.h>
#include <kernel/elf.h>
#include <kernel/ipc.h>
#include <kernel/kmain.h>
#include <kernel/logging.h>
#include <kernel/panic.h>
#include <kernel/process.h>
#include <kernel/syscall.h>
#include <kernel/thread.h>
#include <inttypes.h>
#include <stddef.h>
#include "../build-info.gen.h"


void kmain(const char *cmdline) {
    /* The first thing we want to do is parse the command line options, before
     * we log anything, because some options affect logging, such as whether we
     * need to log to VGA and/or serial port, the baud rate, etc. */
    cmdline_parse_options(cmdline);

    /* Now that we parsed the command line options, we can initialize logging
     * properly and say hello. */
    const cmdline_opts_t *cmdline_opts = cmdline_get_options();
    machine_init_logging(cmdline_opts);

    info("Jinue microkernel started.");
    info("Kernel revision " GIT_REVISION " built " BUILD_TIME " on " BUILD_HOST);
    info("Kernel command line:");
    info("%s", cmdline);
    info("---");

    /* If there were issues parsing the command line, these will be reported
     * here (i.e. panic), now that logging has been initialized and we can log
     * things. */
    cmdline_report_errors();

    /* initialize machine-dependent code */
    machine_init(cmdline_opts);

    kern_mem_block_t ramdisk;
    machine_get_ramdisk(&ramdisk);

    info(
        "Found RAM disk with size %zu bytes at address %#" PRIxKPADDR ".",
        ramdisk.size,
        ramdisk.start
    );

    /* initialize caches */
    ipc_boot_init();
    process_boot_init();

    /* create process for user space loader */
    process_t *process = process_create();

    if(process == NULL) {
        panic("Could not create initial process.");
    }

    process_switch_to(process);

    /* load user space loader binary */
    elf_file_t loader;
    machine_get_loader_elf(&loader);

    elf_info_t elf_info;
    elf_load(&elf_info, loader.ehdr, "jinue-userspace-loader", cmdline, process);

    /* create initial thread */
    thread_t *thread = thread_create(
            process,
            elf_info.entry,
            elf_info.stack_addr
    );
    
    if(thread == NULL) {
        panic("Could not create initial thread.");
    }

    /* This should be the last thing the kernel prints before passing control
     * to the user space loader. */
    info("---");

    /* Start first thread */
    thread_start_first();

    /* should never happen */
    panic("thread_start_first() returned in kmain()");
}
