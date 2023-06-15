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
#include <stdlib.h>
#include "physmem.h"

#define MAX_MAP_ENTRIES 128

static uint64_t alloc_addr;

static uint64_t alloc_limit;

int physmem_init(void) {
    jinue_mem_map_t map;

    int ret = jinue_get_user_memory(&map, sizeof(map), NULL);

    if(ret < 0) {
        return EXIT_FAILURE;
    }

    const jinue_mem_entry_t *entry = NULL;

    for(int idx = 0; idx < map.num_entries; ++idx) {
        entry = &map.entry[idx];
        if(entry->type == JINUE_MEM_TYPE_LOADER_AVAILABLE) {
            break;
        }
    }

    if(entry == NULL || entry->type != JINUE_MEM_TYPE_LOADER_AVAILABLE) {
        return EXIT_FAILURE;
    }

    alloc_addr  = entry->addr;
    alloc_limit = entry->addr + entry->size;

    return EXIT_SUCCESS;
}

int64_t physmem_alloc(size_t size) {
    uint64_t top = alloc_addr + size;

    if(top > alloc_limit) {
        return -1;
    }

    uint64_t retval = alloc_addr;
    alloc_addr      = top;

    return retval;
}
