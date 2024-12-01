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
#include "acpi/acpi.h"
#include "binfmt/elf.h"
#include "core/meminfo.h"
#include "core/server.h"
#include "debug.h"
#include "descriptors.h"
#include "ramdisk.h"
#include "types.h"
#include "utils.h"

#define MAP_BUFFER_SIZE 16384

static jinue_addr_map_t *get_address_map(void *buffer, size_t bufsize) {
    jinue_buffer_t call_buffer;
    call_buffer.addr = buffer;
    call_buffer.size = bufsize;

    int status = jinue_get_address_map(&call_buffer, &errno);

    if(status != 0) {
        jinue_error("error: could not get memory map from kernel: %s", strerror(errno));
        return NULL;
    }

    return buffer;
}

static int get_init(file_t *file, const extracted_ramdisk_t *extracted_ramdisk) {
    const char *init = getenv("init");

    if(init == NULL) {
        init = "/sbin/init";
    }

    const jinue_dirent_t *root      = extracted_ramdisk->root;
    const jinue_dirent_t *dirent    = jinue_dirent_find_by_name(root, init);

    if(dirent == NULL) {
        jinue_error("error: init program not found: %s", init);
        return EXIT_FAILURE;
    }

    if(dirent->type != JINUE_DIRENT_TYPE_FILE) {
        jinue_error("error: init program is not a regular file: %s", init);
        return EXIT_FAILURE;
    }

    uint64_t offset = (char *)jinue_dirent_file(dirent) - (char *)root;
    uint64_t start  = extracted_ramdisk->physaddr + offset;

    file->name          = jinue_dirent_name(dirent);
    file->contents      = jinue_dirent_file(dirent);
    file->size          = dirent->size;
    file->segment_index = add_meminfo_segment(start, dirent->size, JINUE_SEG_TYPE_FILE);

    return EXIT_SUCCESS;
}

static int load_init(
        thread_params_t *thread_params,
        const file_t    *init,
        int              argc,
        char            *argv[]) {

    jinue_info("Loading init program %s", init->name);

    int status = jinue_create_process(INIT_PROCESS_DESCRIPTOR, &errno);

    if(status != 0) {
        jinue_error("error: could not create process for init program: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    status = load_elf(thread_params, init, argc, argv);

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

    status = jinue_create_thread(INIT_THREAD_DESCRIPTOR, INIT_PROCESS_DESCRIPTOR, &errno);

    if (status != 0) {
        jinue_error("error: could not create thread: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    status = jinue_mint(
        INIT_THREAD_DESCRIPTOR,
        INIT_PROCESS_DESCRIPTOR,
        JINUE_DESC_MAIN_THREAD,
        JINUE_PERM_START | JINUE_PERM_AWAIT,
        0,
        &errno
    );

    if (status != 0) {
        jinue_error("error: could not create descriptor for initial thread: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int start_initial_thread(thread_params_t *thread_params) {
    int status = jinue_start_thread(
        INIT_THREAD_DESCRIPTOR,
        thread_params->entry,
        thread_params->stack_addr,
        &errno
    );

    if(status != 0) {
        jinue_error("error: could not start thread: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    status = jinue_close(INIT_THREAD_DESCRIPTOR, &errno);

    if(status != 0) {
        jinue_error("error: could not close thread descriptor: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    jinue_info("Jinue user space loader (%s) started.", argv[0]);

    int status = load_acpi_tables();

    if(status != EXIT_SUCCESS) {
        return status;
    }

    initialize_meminfo();

    char map_buffer[MAP_BUFFER_SIZE];
    jinue_addr_map_t *map = get_address_map(map_buffer, sizeof(map_buffer));

    if(map == NULL) {
        return EXIT_FAILURE;
    }

    ramdisk_t ramdisk;

    status = map_ramdisk(&ramdisk, map);

    if(status != EXIT_SUCCESS) {
        return status;
    }

    extracted_ramdisk_t extracted_ramdisk;
    
    status = extract_ramdisk(&extracted_ramdisk, &ramdisk);

    if(status != EXIT_SUCCESS) {
        return status;
    }

    dump_ramdisk(extracted_ramdisk.root);

    file_t init;
    status = get_init(&init, &extracted_ramdisk);

    if(status != EXIT_SUCCESS) {
        return status;
    }

    thread_params_t thread_params;
    status = load_init(&thread_params, &init, argc, argv);

    if(status != EXIT_SUCCESS) {
        return status;
    }

    jinue_info("---");

    status = start_initial_thread(&thread_params);

    if(status != EXIT_SUCCESS) {
        return status;
    }

    return run_server();
}
