/*
 * Copyright (C) 2026 Philippe Aubertin.
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
#include <sys/mman.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "ramdisk.h"

#define BUFFER_SIZE 16384

int get_ramdisk(ramdisk_t *ramdisk) {
    char buffer[BUFFER_SIZE];

    const jinue_meminfo_t *meminfo = jinue_get_meminfo(buffer, sizeof(buffer));

    if(meminfo == NULL) {
        return EXIT_FAILURE;
    }

    const jinue_segment_t *segment = jinue_get_ramdisk(meminfo);

    if(segment == NULL) {
        return EXIT_FAILURE;
    }

    const jinue_dirent_t *root = mmap(
        NULL,
        segment->size,
        PROT_READ,
        MAP_SHARED,
        -1,
        segment->addr
    );

    if(root == MAP_FAILED) {
        jinue_error("error: mmap() failed: %s.", strerror(errno));
        return EXIT_FAILURE;
    }

    ramdisk->root   = root;
    ramdisk->paddr  = segment->addr;
    ramdisk->size   = segment->size;
    
    return EXIT_SUCCESS;
}

int open_ramdisk_file(file_t *file, const ramdisk_t *ramdisk, const char *filename) {
    const jinue_dirent_t *dirent = jinue_dirent_find_by_name(ramdisk->root, filename);

    if(dirent == NULL) {
        jinue_error("error: file not found: %s", filename);
        return EXIT_FAILURE;
    }

    if(dirent->type != JINUE_DIRENT_TYPE_FILE) {
        jinue_error("error: not a regular file: %s", filename);
        return EXIT_FAILURE;
    }

    uint64_t offset = (const char *)jinue_dirent_file(dirent) - (const char *)ramdisk->root;

    file->name      = jinue_dirent_name(dirent);
    file->contents  = jinue_dirent_file(dirent);
    file->size      = dirent->size;
    file->paddr     = ramdisk->paddr + offset;

    return EXIT_SUCCESS;
}