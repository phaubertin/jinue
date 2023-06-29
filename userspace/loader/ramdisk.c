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
#include <zlib.h>
#include "ramdisk.h"
#include "tar.h"

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

    /* Our implementation of mmap() doesn't actually care about the file descriptor. */
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

static void *zalloc(void *unused, uInt items, uInt size) {
    return malloc(items * size);
}

static void zfree(void *unused, void *address) {
    free(address);
}

int extract_ramdisk(const ramdisk_t *ramdisk) {
    const unsigned char *compressed = ramdisk->addr;

    if(compressed[0] != 0x1f || compressed[1] != 0x8b || compressed[2] != 0x08) {
        jinue_error("error: RAM disk image is not a gzip file (bad signature).");
        return EXIT_FAILURE;
    }

    jinue_info("RAM disk image is a gzip file.");

    z_stream strm;
    strm.zalloc = zalloc;
    strm.zfree = zfree;
    strm.opaque = NULL;
    strm.next_in = compressed;
    strm.avail_in = ramdisk->size;

    int status = inflateInit2(&strm, 16);

    if(status != Z_OK) {
        jinue_error(
                "error: zlib initialization failed: %s",
                strm.msg == NULL ? "(no message)" : strm.msg
        );
        return EXIT_FAILURE;
    }

    unsigned char buffer[512];
    memset(buffer, 0, sizeof(buffer));

    strm.next_out = buffer;
    strm.avail_out = sizeof(buffer);

    status = inflate(&strm, Z_SYNC_FLUSH);

    if(status != Z_OK) {
        jinue_error(
                "error: zlib could not inflate: %s",
                strm.msg == NULL ? "(no message)" : strm.msg
        );
        return EXIT_FAILURE;
    }

    (void)inflateEnd(&strm);

    if(! tar_is_header_valid((const tar_header_t *)buffer)) {
        jinue_error("error: compressed data is not a tar archive (bad signature).");
        return EXIT_FAILURE;
    }

    jinue_info("compressed data is a tar archive");

    return EXIT_SUCCESS;
}
