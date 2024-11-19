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

#include <kernel/domain/entities/endpoint.h>
#include <kernel/domain/entities/process.h>
#include <kernel/domain/entities/thread.h>
#include <kernel/domain/services/cmdline.h>
#include <kernel/domain/services/exec.h>
#include <kernel/domain/services/logging.h>
#include <kernel/domain/services/panic.h>
#include <kernel/domain/config.h>
#include <kernel/machine/init.h>
#include <kernel/kmain.h>
#include <inttypes.h>
#include <stddef.h>
#include "../build-info.gen.h"


void kmain(const char *cmdline) {
    config_t *config = get_config();
    apply_config_defaults(config);

    /* The first thing we want to do is parse the command line options, before
     * we log anything, because some options affect logging, such as whether we
     * need to log to VGA and/or serial port, the baud rate, etc. */
    cmdline_parse_options(config, cmdline);

    /* Now that we parsed the command line options, we can initialize logging
     * properly and say hello. */
    machine_init_logging(config);

    info("Jinue microkernel started.");
    info("Kernel revision " GIT_REVISION " built " BUILD_TIME " on " BUILD_HOST);
    info("Kernel command line:");
    info("%s", cmdline);
    info("---");

    /* If there were issues parsing the command line, these will be reported
     * here (i.e. panic), now that logging has been initialized and we can log
     * things to the right places. */
    cmdline_report_errors();

    /* initialize machine-dependent code */
    machine_init(config);

    kern_mem_block_t ramdisk;
    machine_get_ramdisk(&ramdisk);

    info(
        "Found RAM disk with size %zu bytes at address %#" PRIxKPADDR ".",
        ramdisk.size,
        ramdisk.start
    );

    /* initialize caches */
    initialize_endpoint_cache();
    initialize_process_cache();

    /* create process for user space loader */
    process_t *process = construct_process();

    if(process == NULL) {
        panic("Could not create initial process.");
    }

    switch_to_process(process);

    /* create user space loader main thread */
    thread_t *thread = construct_thread(process);

    if(thread == NULL) {
        panic("Could not create initial thread.");
    }

    /* load user space loader binary */
    exec_file_t loader;
    machine_get_loader(&loader);

    exec(
        process,
        thread,
        &loader,
        "jinue-userspace-loader",
        cmdline
    );

    /* This should be the last thing the kernel prints before passing control
     * to the user space loader. */
    info("---");

    /* Start first thread. */
    run_first_thread(thread);

    /* should never happen */
    panic("run_first_thread() returned in kmain()");
}
