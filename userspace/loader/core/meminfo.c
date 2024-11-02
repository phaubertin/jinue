/*
 * Copyright (C) 2024 Philippe Aubertin.
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

#include <jinue/loader.h>
#include <jinue/utils.h>
#include <errno.h>
#include <internals.h>
#include <stdlib.h>
#include <string.h>
#include "meminfo.h"
#include "server.h"

#define MAX_SEGMENTS    8

#define MAX_VMAPS       8

static jinue_loader_meminfo_t meminfo;

static jinue_loader_segment_t segments[MAX_SEGMENTS];

static jinue_loader_mapping_t mappings[MAX_VMAPS];

static jinue_const_buffer_t buffers[] = {
#define BUFFER_INDEX_MEMINFO    0
    {
        .addr = &meminfo,
        .size = sizeof(meminfo)
    },
#define BUFFER_INDEX_SEGMENTS   1
    {
        .addr = segments,
        .size = 0
    },
#define BUFFER_INDEX_VMAPS      2
    {
        .addr = mappings,
        .size = 0
    }
};

#define SEGMENTS_SIZE   (meminfo.n_segments * sizeof(jinue_loader_segment_t))

#define MAPPINGS_SIZE   (meminfo.n_mappings * sizeof(jinue_loader_mapping_t))

#define MESSAGE_SIZE    (sizeof(meminfo) + SEGMENTS_SIZE + MAPPINGS_SIZE)

void initialize_meminfo(void) {
    memset(&meminfo, 0, sizeof(meminfo));
    memset(segments, 0, sizeof(segments));
    memset(mappings, 0, sizeof(mappings));
}

int add_meminfo_segment(uint64_t addr, uint64_t size, int type) {
    int index = meminfo.n_segments++;

    segments[index].addr = addr;
    segments[index].size = size;
    segments[index].type = type;

    return index;
}

uint64_t get_meminfo_segment_start(int index) {
    return segments[index].addr;
}

void set_meminfo_ramdisk(uint64_t addr, uint64_t size) {
    meminfo.ramdisk = add_meminfo_segment(addr, size, JINUE_SEG_TYPE_RAMDISK);
}

uint64_t get_meminfo_ramdisk_start(void) {
    return get_meminfo_segment_start(meminfo.ramdisk);
}

void add_meminfo_mapping(void *addr, size_t size, int segment_index, size_t offset, int perms) {
    int index = meminfo.n_mappings++;

    mappings[index].addr       = addr;
    mappings[index].size       = size;
    mappings[index].segment    = segment_index;
    mappings[index].offset     = offset;
    mappings[index].perms      = perms;
}

static void update_meminfo(void) {
    meminfo.hints.physaddr  = _libc_get_physmem_alloc_addr();
    meminfo.hints.physlimit = _libc_get_physmem_alloc_limit();
}

static void update_buffers(void) {
    buffers[BUFFER_INDEX_SEGMENTS].size = SEGMENTS_SIZE;
    buffers[BUFFER_INDEX_VMAPS].size    = MAPPINGS_SIZE;
}

int get_meminfo(const jinue_message_t *message) {
    if(message->reply_max_size < MESSAGE_SIZE) {
        return reply_error(JINUE_E2BIG);
    }

    update_meminfo();

    update_buffers();

    jinue_message_t reply;
    reply.send_buffers          = buffers;
    reply.send_buffers_length   = sizeof(buffers)/sizeof(buffers[0]);

    int status = jinue_reply(&reply, &errno);

    if(status < 0) {
        jinue_error("jinue_reply() failed: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
