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

#include <jinue/jinue.h>
#include <jinue/loader.h>
#include <jinue/utils.h>
#include <sys/mman.h>
#include <errno.h>
#include <internals.h>
#include <stdint.h>
#include <string.h>
#include "../descriptors.h"
#include "mappings.h"
#include "meminfo.h"

int map_file(void *vaddr, size_t size, int segment_index, size_t offset, int perms) {
    uint64_t file_start = get_meminfo_segment_start(segment_index);
    uint64_t paddr      = file_start + offset;

    int status = jinue_mmap(INIT_PROCESS_DESCRIPTOR, vaddr, size, perms, paddr, &errno);

    if(status < 0) {
        jinue_error("error: jinue_mmap() failed: %s", strerror(errno));
        return status;
    }

    add_meminfo_vmap(vaddr, size, segment_index, offset, perms);

    return 0;
}

void *map_anonymous(void *vaddr, size_t size, int perms) {
    uint64_t paddr = _libc_get_physmem_alloc_addr();

    /* Map into this process so we can set the contents. */
    void *segment = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if(segment == MAP_FAILED) {
        jinue_error("error: mmap() failed: %s", strerror(errno));
        return NULL;
    }

    /* Map into the target process. */
    int status = jinue_mmap(INIT_PROCESS_DESCRIPTOR, vaddr, size, perms, paddr, &errno);

    if(status < 0) {
        jinue_error("error: jinue_mmap() failed: %s", strerror(errno));
        return NULL;
    }

    int index = add_meminfo_segment(paddr, size, JINUE_SEG_TYPE_ANON);
    
    add_meminfo_vmap(vaddr, size, index, 0, perms);

    return segment;
}
