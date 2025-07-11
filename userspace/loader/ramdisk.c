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
#include <jinue/utils.h>
#include <sys/mman.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "archives/tar.h"
#include "core/meminfo.h"
#include "streams/bzip2.h"
#include "streams/bzip2.h"
#include "streams/gzip.h"
#include "streams/raw.h"
#include "streams/stream.h"
#include "ramdisk.h"
#include "utils.h"

struct stream_initializer {
    const char *name;
    int (*initialize)(stream_t *, const void *, size_t);
};

static struct stream_initializer stream_initializers[] = {
    {"bzip2",   bzip2_stream_initialize},
    {"gzip",    gzip_stream_initialize},
    {NULL, NULL},
};

/**
 * Map the compressed RAM disk image in this process
 *
 * @param ramdisk structure to initialize with RAM disk image address and size
 * @param map kernel memory map
 * @return EXIT_SUCCESS on success, other value on failure
 *
 * */
int map_ramdisk(ramdisk_t *ramdisk, const jinue_addr_map_t *map) {
    const jinue_addr_map_entry_t *ramdisk_entry = get_addr_map_entry_by_type(
        map,
        JINUE_MEMYPE_RAMDISK
    );

    if(ramdisk_entry == NULL || ramdisk_entry->addr == 0 || ramdisk_entry->size == 0) {
        jinue_error("error: no initial RAM disk found.");
        return EXIT_FAILURE;
    }

    jinue_info(
        "Found RAM disk with size %" PRIu64 " bytes at address %#" PRIx64 ".",
        ramdisk_entry->size,
        ramdisk_entry->addr);

    /* TODO be more flexible on this */
    if((ramdisk_entry->addr & (PAGE_SIZE - 1)) != 0) {
        jinue_error("error: RAM disk is not aligned on a page boundary");
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
        jinue_error("error: could not map RAM disk: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    ramdisk->addr = addr;
    ramdisk->size = ramdisk_entry->size;

    return EXIT_SUCCESS;
}

/**
 * Initialize stream to read uncompressed RAM disk
 *
 * This function detects the compression algorithm and initializes the right
 * type of stream to read the uncompressed RAM disk data.
 *
 * @param stream stream to initialize
 * @param ramdisk structure that contains the RAM disk image address and size
 * @return STREAM_SUCCESS on success, other value on failure
 *
 * */
static int initialize_stream(stream_t *stream, const ramdisk_t *ramdisk) {
    for(int idx = 0; stream_initializers[idx].initialize != NULL; ++idx) {
        const struct stream_initializer *initializer = &stream_initializers[idx];

        int status = initializer->initialize(stream, ramdisk->addr, ramdisk->size);

        if(status == STREAM_SUCCESS) {
            jinue_info("RAM disk image is compressed with %s.", initializer->name);
            return STREAM_SUCCESS;
        }

        if(status != STREAM_FORMAT) {
            return status;
        }
    }

    jinue_info("RAM disk image is uncompressed.");

    return raw_stream_initialize(stream, ramdisk->addr, ramdisk->size);
}

enum format {
    FORMAT_TAR,
    FORMAT_UNKNOWN
};

/**
 * Detect the format of the RAM disk archive
 *
 * The stream abstract the compression algorithm, if application, so this
 * function determines the archive format after decompression.
 *
 * @param stream stream that represents the archive
 * @return archive format, FORMAT_UNKNOWN if unknown
 *
 * */
static enum format detect_format(stream_t *stream) {
    bool tar = is_tar(stream);

    stream_reset(stream);

    if(tar) {
        return FORMAT_TAR;
    }

    return FORMAT_UNKNOWN;
}

/**
 * Extract the initial RAM disk into a virtual filesystem
 *
 * The following formats are currently supported:
 *
 * Archive format:
 * - tar archive
 *
 * Compression algorithms:
 * - no compression (.tar)
 * - gzip compression (.tar.gz)
 *
 * @param extracted extracted RAM disk address and size (out)
 * @param ramdisk RAM disk image address and size
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure
 *
 * */
int extract_ramdisk(extracted_ramdisk_t *extracted, const ramdisk_t *ramdisk) {
    stream_t stream;

    int status = initialize_stream(&stream, ramdisk);

    if(status != STREAM_SUCCESS) {
        return EXIT_FAILURE;
    }

    switch(detect_format(&stream)) {
    case FORMAT_TAR:
        jinue_info("RAM disk is a tar archive.");
        status = tar_extract(extracted, &stream);
        break;
    default:
        jinue_error("error: could not extract RAM disk: unrecognized format");
        status = EXIT_FAILURE;
        break;
    }

    stream_finalize(&stream);

    if(status != EXIT_SUCCESS) {
        return status;
    }
    
    set_meminfo_ramdisk(extracted->physaddr, extracted->size);

    return EXIT_SUCCESS;
}
