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
#include <errno.h>
#include <internals.h>
#include <stdlib.h>
#include "physmem.h"

#define BUFFER_SIZE 2048

static struct {
    uint64_t addr;
    uint64_t limit;
} alloc_range;

static void initialize_range(uint64_t addr, uint64_t limit) {
    alloc_range.addr    = addr;
    alloc_range.limit   = limit;
}

static int initialize_range_from_loader_info(void) {
    char buffer[BUFFER_SIZE];

    jinue_buffer_t reply_buffer;
    reply_buffer.addr = buffer;
    reply_buffer.size = sizeof(buffer);

    jinue_message_t message;
    message.send_buffers        = NULL;
    message.send_buffers_length = 0;
    message.recv_buffers        = &reply_buffer;
    message.recv_buffers_length = 1;

    int status = jinue_send(
        JINUE_DESC_LOADER_ENDPOINT,
        JINUE_MSG_GET_MEMINFO,
        &message,
        &errno,
        NULL
    );

    if(status < 0) {
        /* errno is set by call to jinue_send(). */
        return status;
    }

    const jinue_meminfo_t *meminfo = (const jinue_meminfo_t *)buffer;

    initialize_range(meminfo->hints.physaddr, meminfo->hints.physlimit);

    return status;
}

static const jinue_addr_map_entry_t *find_range_by_type(const jinue_addr_map_t *map, int type) {
    for(int idx = 0; idx < map->num_entries; ++idx) {
        const jinue_addr_map_entry_t *entry = &map->entry[idx];

        if(entry->type == type) {
            return entry;
        }
    }

    return NULL;
}

static int initialize_range_from_kernel_info(void) {
    char map_buffer[BUFFER_SIZE];

    jinue_buffer_t call_buffer;
    call_buffer.addr = map_buffer;
    call_buffer.size = sizeof(map_buffer);

    int status = jinue_get_address_map(&call_buffer, NULL);

    if(status < 0) {
        return EXIT_FAILURE;
    }

    const jinue_addr_map_t *map         = (const jinue_addr_map_t *)map_buffer;
    const jinue_addr_map_entry_t *entry = find_range_by_type(map, JINUE_MEMYPE_LOADER_AVAILABLE);

    if(entry == NULL) {
        return EXIT_FAILURE;
    }

    initialize_range(entry->addr, entry->addr + entry->size);

    return EXIT_SUCCESS;
}

int __physmem_init(void) {
    int status = initialize_range_from_loader_info();

    if(status >= 0) {
        return EXIT_SUCCESS;
    }

    if(errno != JINUE_EBADF) {
        return EXIT_FAILURE;
    }

    /* We weren't able to get the memory usage information from the loader, most likely because
     * we *are* the loader. Fall back to the kernel call, which provides information about memory
     * available to user space, but not about which parts of that user space memory have already
     * been allocated. */
    return initialize_range_from_kernel_info();
}

int64_t __physmem_alloc(size_t size) {
    uint64_t top = alloc_range.addr + size;

    if(top > alloc_range.limit) {
        return -1;
    }

    uint64_t retval     = alloc_range.addr;
    alloc_range.addr    = top;

    return retval;
}

uint64_t __get_physmem_alloc_addr(void) {
    return alloc_range.addr;
}

uint64_t __get_physmem_alloc_limit(void) {
    return alloc_range.limit;
}
