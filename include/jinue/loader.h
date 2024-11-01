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

#ifndef _JINUE_LOADER_H
#define _JINUE_LOADER_H

#include <jinue/shared/asm/messages.h>
#include <stddef.h>
#include <stdint.h>

/** normal file */
#define JINUE_DIRENT_TYPE_FILE      1

/** directory */
#define JINUE_DIRENT_TYPE_DIR       2

/** symbolic link */
#define JINUE_DIRENT_TYPE_SYMLINK   3

/** character device */
#define JINUE_DIRENT_TYPE_CHARDEV   4

/** block device */
#define JINUE_DIRENT_TYPE_BLKDEV    5

/** FIFO */
#define JINUE_DIRENT_TYPE_FIFO      6


/** link to continuation of list - for internal use by implementation */
#define JINUE_DIRENT_TYPE_NEXT  (-1)

/** end of list - for internal use by implementation */
#define JINUE_DIRENT_TYPE_END   (-2)

#define JINUE_ISUID             (1<<11)

#define JINUE_ISGID             (1<<10)

#define JINUE_IRUSR             (1<<8)

#define JINUE_IWUSR             (1<<7)

#define JINUE_IXUSR             (1<<6)

#define JINUE_IRGRP             (1<<5)

#define JINUE_IWGRP             (1<<4)

#define JINUE_IXGRP             (1<<3)

#define JINUE_IROTH             (1<<2)

#define JINUE_IWOTH             (1<<1)

#define JINUE_IXOTH             (1<<0)

struct jinue_dirent {
    uint32_t    type;
    uint32_t    mode;
    uint32_t    uid;
    uint32_t    gid;
    uint32_t    devmajor;
    uint32_t    devminor;
    uint64_t    size;
    int64_t     rel_name;
    int64_t     rel_value;
};

typedef struct jinue_dirent jinue_dirent_t;

const jinue_dirent_t *jinue_dirent_get_first(const jinue_dirent_t *root);

const jinue_dirent_t *jinue_dirent_get_next(const jinue_dirent_t *prev);

const jinue_dirent_t *jinue_dirent_find_by_name(const jinue_dirent_t *root, const char *name);

const char *jinue_dirent_name(const jinue_dirent_t *dirent);

const void *jinue_dirent_file(const jinue_dirent_t *dirent);

const char *jinue_dirent_link(const jinue_dirent_t *dirent);

typedef struct {
    int n_segments;
    int n_vmaps;
    int ramdisk;
    struct {
        uint64_t     physaddr;
        uint64_t     physlimit;
    } hints;
} jinue_loader_meminfo_t;

/** extracted RAM disk */
#define JINUE_SEG_TYPE_RAMDISK  0

/** file loaded by the loader */
#define JINUE_SEG_TYPE_FILE     1

/** anonymous memory */
#define JINUE_SEG_TYPE_ANON     2

/** other memory referenced by a mapping */
#define JINUE_SEG_TYPE_OTHER    3

typedef struct {
    uint64_t    addr;
    uint64_t    size;
    int         type;
} jinue_loader_segment_t;

typedef struct {
    void    *addr;
    size_t   size;
    int      perms;
    int      segment;
} jinue_loader_vmap_t;

#endif
