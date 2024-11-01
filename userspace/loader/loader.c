/*
 * Copyright (C) 2023-2024 Philippe Aubertin.
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
#include <jinue/utils.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "binfmt/elf.h"
#include "server/meminfo.h"
#include "server/server.h"
#include "debug.h"
#include "descriptors.h"
#include "ramdisk.h"
#include "utils.h"

#define MAP_BUFFER_SIZE 16384

static jinue_mem_map_t *get_memory_map(void *buffer, size_t bufsize) {
    int status = jinue_get_user_memory(buffer, bufsize, &errno);

    if(status != 0) {
        jinue_error("error: could not get memory map from kernel: %s", strerror(errno));
        return NULL;
    }

    return buffer;
}

static const jinue_dirent_t *get_init(const jinue_dirent_t *root) {
    const char *init = getenv("init");

    if(init == NULL) {
        init = "/sbin/init";
    }

    const jinue_dirent_t *dirent = jinue_dirent_find_by_name(root, init);

    if(dirent == NULL) {
        jinue_error("error: init program not found: %s", init);
        return NULL;
    }

    if(dirent->type != JINUE_DIRENT_TYPE_FILE) {
        jinue_error("error: init program is not a regular file: %s", init);
        return NULL;
    }

    return dirent;
}

static int load_init(elf_info_t *elf_info, const jinue_dirent_t *init, int argc, char *argv[]) {
    jinue_info("Loading init program %s", jinue_dirent_name(init));

    int status = jinue_create_process(INIT_PROCESS_DESCRIPTOR, &errno);

    if(status != 0) {
        jinue_error("error: could not create process for init program: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    status = load_elf(elf_info, INIT_PROCESS_DESCRIPTOR, init, argc, argv);

    if(status != EXIT_SUCCESS) {
        return status;
    }

    status = jinue_mint(
        INIT_PROCESS_DESCRIPTOR,
        INIT_PROCESS_DESCRIPTOR,
        JINUE_DESC_SELF_PROCESS,
        JINUE_PERM_CREATE_THREAD | JINUE_PERM_MAP | JINUE_PERM_OPEN,
        0,
        &errno
    );

    if (status != 0) {
        jinue_error("error: could not create self process descriptor: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    status = jinue_create_endpoint(JINUE_DESC_LOADER_ENDPOINT, &errno);

    if (status != 0) {
        jinue_error("error: could not create endpoint: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    status = jinue_mint(
        JINUE_DESC_LOADER_ENDPOINT,
        INIT_PROCESS_DESCRIPTOR,
        JINUE_DESC_LOADER_ENDPOINT,
        JINUE_PERM_SEND,
        0,
        &errno
    );

    if (status != 0) {
        jinue_error("error: could not create descriptor for endpoint: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    jinue_info("Jinue user space loader (%s) started.", argv[0]);

    initialize_meminfo();

    char map_buffer[MAP_BUFFER_SIZE];
    jinue_mem_map_t *map = get_memory_map(map_buffer, sizeof(map_buffer));

    if(map == NULL) {
        return EXIT_FAILURE;
    }

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

    const jinue_dirent_t *init = get_init(root);

    if(init == NULL) {
        return EXIT_FAILURE;
    }

    elf_info_t elf_info;
    status = load_init(&elf_info, init, argc, argv);

    if(status != EXIT_SUCCESS) {
        return status;
    }

    jinue_info("---");

    status = jinue_create_thread(INIT_THREAD_DESCRIPTOR, INIT_PROCESS_DESCRIPTOR, &errno);

    if (status != 0) {
        jinue_error("error: could not create thread: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    status = jinue_start_thread(
        INIT_THREAD_DESCRIPTOR,
        elf_info.entry,
        elf_info.stack_addr,
        &errno
    );

    if (status != 0) {
        jinue_error("error: could not start thread: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    status = jinue_close(INIT_THREAD_DESCRIPTOR, &errno);

    if (status != 0) {
        jinue_error("error: could not close thread descriptor: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    return run_server();
}
