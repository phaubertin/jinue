/*
 * Copyright (C) 2023 Philippe Aubertin.
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

#include <jinue/jinue.h>
#include <jinue/loader.h>
#include <jinue/util.h>
#include <stdlib.h>
#include "debug.h"
#include "ramdisk.h"
#include "util.h"

#define MAP_BUFFER_SIZE     16384

static jinue_mem_map_t *get_memory_map(void *buffer, size_t bufsize) {
    int status = jinue_get_user_memory((jinue_mem_map_t *)buffer, bufsize, NULL);

    if(status != 0) {
        jinue_error("error: could not get physical memory map from microkernel");
        return NULL;
    }

    return buffer;
}

int main(int argc, char *argv[]) {
    /* Say hello. */
    jinue_info("Jinue user space loader (%s) started.", argv[0]);

    dump_cmdline_arguments(argc, argv);
    dump_environ();
    dump_auxvec();
    dump_syscall_implementation();

    char map_buffer[MAP_BUFFER_SIZE];
    jinue_mem_map_t *map = get_memory_map(map_buffer, sizeof(map_buffer));

    if(map == NULL) {
        return EXIT_FAILURE;
    }

    dump_phys_memory_map(map);

    ramdisk_t ramdisk;

    int status = map_ramdisk(&ramdisk, map);

    if(status != EXIT_SUCCESS) {
        return status;
    }

    const jinue_dirent_t *root = extract_ramdisk(&ramdisk);

    if(root == NULL) {
        return EXIT_FAILURE;
    }

    dump_ramdisk(root);

    jinue_info("---");

    if(bool_getenv("DEBUG_DO_REBOOT")) {
        jinue_info("Rebooting.");
        jinue_reboot();
    }

    while (1) {
        jinue_yield_thread();
    }
    
    return EXIT_SUCCESS;
}
