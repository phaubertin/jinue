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

#ifndef _JINUE_INITRD_H
#define _JINUE_INITRD_H

#include <stddef.h>

#define JINUE_DIRENT_TYPE_FILE  1

#define JINUE_DIRENT_TYPE_NEXT  2

#define JINUE_DIRENT_TYPE_END   3


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


typedef struct {
    int          type;
    int          mode;
    int          uid;
    int          gid;
    size_t       offset;
    const char  *name;
} jinue_dirent_t;

#endif
