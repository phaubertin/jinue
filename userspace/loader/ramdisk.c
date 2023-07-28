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
#include <jinue/util.h>
#include <sys/mman.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "archives/tar.h"
#include "streams/gzip.h"
#include "streams/raw.h"
#include "streams/stream.h"
#include "ramdisk.h"


static const jinue_mem_entry_t *get_ramdisk_entry(const jinue_mem_map_t *map) {
    for(int idx = 0; idx < map->num_entries; ++idx) {
        if(map->entry[idx].type == JINUE_MEM_TYPE_RAMDISK) {
            return &map->entry[idx];
        }
    }

    return NULL;
}

int map_ramdisk(ramdisk_t *ramdisk, const jinue_mem_map_t *map) {
    const jinue_mem_entry_t *ramdisk_entry = get_ramdisk_entry(map);

    if(ramdisk_entry == NULL || ramdisk_entry->addr == 0 || ramdisk_entry->size == 0) {
        jinue_error("No initial RAM disk found.");
        return EXIT_FAILURE;
    }

    jinue_info(
        "Found RAM disk with size %" PRIu64 " bytes at address %#" PRIx64 ".",
        ramdisk_entry->size,
        ramdisk_entry->addr);

    /* TODO be more flexible on this */
    if((ramdisk_entry->addr & (PAGE_SIZE - 1)) != 0) {
        jinue_error("error: RAM disk is not aligned on a page boundary.");
        return EXIT_FAILURE;
    }

    /* Our implementation of mmap() doesn't actually care about the file descriptor,
     * the "file" is always physical memory (aka. /dev/mem). */
    const int dummy_fd = 0;

    const unsigned char *addr = mmap(
            NULL,
            ramdisk_entry->size,
            PROT_READ,
            MAP_SHARED,
            dummy_fd,
            ramdisk_entry->addr);

    if(addr == MAP_FAILED) {
        jinue_error("error: could not map RAM disk (%i).", errno);
        return NULL;
    }

    ramdisk->addr = addr;
    ramdisk->size = ramdisk_entry->size;

    return EXIT_SUCCESS;
}

static int initialize_stream(stream_t *stream, const ramdisk_t *ramdisk) {
    int status = gzip_stream_initialize(stream, ramdisk->addr, ramdisk->size);

    if(status == STREAM_SUCCESS) {
        jinue_info("RAM disk image is compressed with gzip.");
        return STREAM_SUCCESS;
    }

    if(status != STREAM_FORMAT) {
        return status;
    }

    jinue_info("RAM disk image is uncompressed.");

    return raw_stream_initialize(stream, ramdisk->addr, ramdisk->size);
}

enum format {
    FORMAT_TAR,
    FORMAT_UNKNOWN
};

static enum format detect_format(stream_t *stream) {
    bool tar = is_tar(stream);

    stream_reset(stream);

    if(tar) {
        return FORMAT_TAR;
    }

    return FORMAT_UNKNOWN;
}

const jinue_dirent_t *extract_ramdisk(const ramdisk_t *ramdisk) {
    stream_t stream;

    int status = initialize_stream(&stream, ramdisk);

    if(status != STREAM_SUCCESS) {
        return NULL;
    }

    const jinue_dirent_t *root;

    switch(detect_format(&stream)) {
    case FORMAT_TAR:
        jinue_info("RAM disk is a tar archive.");
        root = tar_extract(&stream);
        break;
    default:
        jinue_error("error: could not extract RAM disk: unrecognized format");
        root = NULL;
        break;
    }

    stream_finalize(&stream);

    return root;
}
