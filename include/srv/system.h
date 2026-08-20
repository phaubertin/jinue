/*
 * Copyright (C) 2026 Philippe Aubertin.
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

#ifndef _JINUE_LIBC_SRV_SYSTEM_H
#define _JINUE_LIBC_SRV_SYSTEM_H

#include <jinue/jinue.h>
#include <stddef.h>

/* Descriptors */

#define SYS_DESC_SELF_PROCESS   JINUE_DESC_SELF_PROCESS

#define SYS_DESC_MAIN_THREAD    JINUE_DESC_MAIN_THREAD

#define SYS_DESC_ENDPOINT       JINUE_DESC_LOADER_ENDPOINT

/* Message types */

#define SYS_MSG_EXIT        (JINUE_SYS_USER_BASE + 0)

#define SYS_MSG_MAP_ANON    (JINUE_SYS_USER_BASE + 1)

/* Message structures */

typedef struct {
    void    *addr;
    size_t   length;
    int      prot;
} sys_msg_map_anon_params_t;

#endif
