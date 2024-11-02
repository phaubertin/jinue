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
#include <jinue/utils.h>
#include <inttypes.h>
#include "debug.h"
#include "utils.h"


#define PRETTY_MODE_SIZE 11

static const char *pretty_mode(char *mode, const jinue_dirent_t *dirent) {
    switch(dirent->type) {
    case JINUE_DIRENT_TYPE_FILE:
        mode[0] = '-';
        break;
    case JINUE_DIRENT_TYPE_DIR:
        mode[0] = 'd';
        break;
    case JINUE_DIRENT_TYPE_SYMLINK:
        mode[0] = '1';
        break;
    case JINUE_DIRENT_TYPE_CHARDEV:
        mode[0] = 'c';
        break;
    case JINUE_DIRENT_TYPE_BLKDEV:
        mode[0] = 'b';
        break;
    case JINUE_DIRENT_TYPE_FIFO:
        mode[0] = 'p';
        break;
    default:
        mode[0] = '?';
        break;
    }

    struct {int pos; int c; int mask;} map[] = {
            {1, 'r', JINUE_IRUSR},
            {2, 'w', JINUE_IWUSR},
            {3, 's', JINUE_IXUSR | JINUE_ISUID},
            {3, 'S', JINUE_ISUID},
            {3, 'x', JINUE_IXUSR},
            {4, 'r', JINUE_IRGRP},
            {5, 'w', JINUE_IWGRP},
            {6, 's', JINUE_IXGRP | JINUE_ISGID},
            {6, 'S', JINUE_ISGID},
            {6, 'x', JINUE_IXGRP},
            {7, 'r', JINUE_IROTH},
            {8, 'w', JINUE_IWOTH},
            {9, 'x', JINUE_IXOTH}
    };

    for(int idx = 0; idx < sizeof(map) / sizeof(map[0]); ++idx) {
        if((dirent->mode & map[idx].mask) == map[idx].mask) {
            mode[map[idx].pos] = map[idx].c;
        } else {
            mode[map[idx].pos] = '-';
        }
    }

    mode[10] = '\0';

    return mode;
}

void dump_ramdisk(const jinue_dirent_t *root) {
    char mode_buffer[PRETTY_MODE_SIZE];

    if(! bool_getenv("DEBUG_DUMP_RAMDISK")) {
        return;
    }

    jinue_info("RAM disk dump:");

    const jinue_dirent_t *dirent = jinue_dirent_get_first(root);

    while(dirent != NULL) {
        jinue_info(
                "  %s %6" PRIu32 " %6" PRIu32 " %12" PRIu64 " %s",
                pretty_mode(mode_buffer, dirent),
                dirent->uid,
                dirent->gid,
                dirent->size,
                jinue_dirent_name(dirent)
        );

        dirent = jinue_dirent_get_next(dirent);
    }
}
