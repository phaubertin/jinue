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
#include <sys/mman.h>
#include <errno.h>
#include <internals.h>
#include <limits.h>
#include "mmap.h"
#include "physmem.h"

static void *alloc_addr = (void *)MMAP_BASE;

void *mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off) {
    return __mmap_perrno(addr, len, prot, flags, fildes, off, &errno);
}

void *__mmap_perrno(
        void    *addr,
        size_t   len,
        int      prot,
        int      flags,
        int      fildes,
        off_t    off,
        int     *perrno) {

    if((flags & (MAP_SHARED | MAP_PRIVATE)) == 0) {
        *perrno = EINVAL;
        return MAP_FAILED;
    }

    if(flags & MAP_PRIVATE) {
        *perrno = ENOTSUP;
        return MAP_FAILED;
    }

    const int flags_mask = MAP_FIXED | MAP_SHARED | MAP_ANONYMOUS;

    if((flags & ~flags_mask) != 0) {
        *perrno = EINVAL;
        return MAP_FAILED;
    }

    const int prot_mask = PROT_READ | PROT_WRITE | PROT_EXEC;

    if((prot & ~prot_mask) != 0) {
        *perrno = EINVAL;
        return MAP_FAILED;
    }

    const int write_exec = PROT_WRITE | PROT_EXEC;

    if((prot & write_exec) == write_exec) {
        *perrno = ENOTSUP;
        return MAP_FAILED;
    }

    if(len == 0) {
        *perrno = EINVAL;
        return MAP_FAILED;
    }

    size_t aligned_length = (len + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    if(flags & MAP_FIXED) {
        if(addr == NULL || ((uintptr_t)addr & (PAGE_SIZE - 1)) != 0) {
            *perrno = EINVAL;
            return MAP_FAILED;
        }
    }
    else {
        addr = alloc_addr;
    }

    if((uintptr_t)addr >= JINUE_KLIMIT || JINUE_KLIMIT - (uintptr_t)addr < aligned_length) {
        if(flags & MAP_FIXED) {
            *perrno = EINVAL;
        }
        else {
            *perrno = ENOMEM;
        }
        return MAP_FAILED;
    }

    if(off < 0 || (off & (PAGE_SIZE - 1)) != 0) {
        *perrno = EINVAL;
        return MAP_FAILED;
    }

    int64_t paddr;

    if(flags & MAP_ANONYMOUS) {
        paddr = __physmem_alloc(aligned_length);

        if(paddr < 0) {
            *perrno = ENOMEM;
            return MAP_FAILED;
        }
    }
    else {
        paddr = off;
    }

    int ret = jinue_mmap(JINUE_DESC_SELF_PROCESS, addr, aligned_length, prot, paddr, perrno);

    if(ret < 0) {
        return MAP_FAILED;
    }

    if(!(flags & MAP_FIXED)) {
        alloc_addr = (void *)((uintptr_t)alloc_addr + aligned_length);
    }

    return addr;
}

void *__mmap_anonymous(void *addr, size_t len) {
    return __mmap_anonymous_perrno(addr, len, &errno);
}

void *__mmap_anonymous_perrno(void *addr, size_t len, int *perrno) {
    return __mmap_perrno(
        addr,
        len,
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0,
        perrno
    );
}
