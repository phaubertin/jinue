/*
 * Copyright (C) 2023-2024 Philippe Aubertin.
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
#include <stddef.h>
#include <string.h>

const jinue_dirent_t *jinue_dirent_get_first(const jinue_dirent_t *root) {
    if(root == NULL || root->type == JINUE_DIRENT_TYPE_END) {
        return NULL;
    }

    return root;
}

const jinue_dirent_t *jinue_dirent_get_next(const jinue_dirent_t *prev) {
    if(prev == NULL || prev->type == JINUE_DIRENT_TYPE_END) {
        return NULL;
    }

    const jinue_dirent_t *current = prev + 1;

    if(current->type == JINUE_DIRENT_TYPE_NEXT) {
        current = (const jinue_dirent_t *)((const char *)current + current->rel_value);
    }

    if(current->type == JINUE_DIRENT_TYPE_END) {
        return NULL;
    }

    return current;
}

const jinue_dirent_t *jinue_dirent_find_by_name(const jinue_dirent_t *root, const char *name) {
    const jinue_dirent_t *dirent = jinue_dirent_get_first(root);

    while(dirent != NULL) {
        if(strcmp(jinue_dirent_name(dirent), name) == 0) {
            return dirent;
        }

        dirent = jinue_dirent_get_next(dirent);
    }

    return NULL;
}

const char *jinue_dirent_name(const jinue_dirent_t *dirent) {
    return (const char *)dirent + dirent->rel_name;
}

const void *jinue_dirent_file(const jinue_dirent_t *dirent) {
    return (const char *)dirent + dirent->rel_value;
}

const char *jinue_dirent_link(const jinue_dirent_t *dirent) {
    return (const char *)dirent + dirent->rel_value;
}

const jinue_meminfo_t *jinue_get_meminfo(void *buffer, size_t bufsize) {
    jinue_buffer_t reply_buffer;
    reply_buffer.addr = buffer;
    reply_buffer.size = bufsize;

    jinue_message_t message;
    message.send_buffers        = NULL;
    message.send_buffers_length = 0;
    message.recv_buffers        = &reply_buffer;
    message.recv_buffers_length = 1;

    int errnum;
    uintptr_t errcode;

    int status = jinue_send(
        JINUE_DESC_LOADER_ENDPOINT,
        JINUE_MSG_GET_MEMINFO,
        &message,
        &errnum,
        &errcode
    );

    if(status < 0) {
        if(errnum == JINUE_EPROTO) {
            jinue_error("error: loader set error code to: %s.", strerror(errcode));
        } else {
            jinue_error("error: jinue_send() failed: %s.", strerror(errnum));
        }

        return NULL;
    }

    return buffer;
}

const jinue_segment_t *jinue_get_segment(const jinue_meminfo_t *meminfo, unsigned int index) {
    if(meminfo == NULL) {
        return NULL;
    }

    if(index >= meminfo->n_segments) {
        return NULL;
    }

    return &((const jinue_segment_t *)&meminfo[1])[index];
}

const jinue_segment_t *jinue_get_ramdisk(const jinue_meminfo_t *meminfo) {
    if(meminfo == NULL) {
        return NULL;
    }

    return jinue_get_segment(meminfo, meminfo->ramdisk);
}

const jinue_mapping_t *jinue_get_mapping(const jinue_meminfo_t *meminfo, unsigned int index) {
    if(meminfo == NULL) {
        return NULL;
    }

    if(index >= meminfo->n_mappings) {
        return NULL;
    }

    const jinue_segment_t *segments = (const jinue_segment_t *)&meminfo[1];
    return &((jinue_mapping_t *)&segments[meminfo->n_segments])[index];
}

int jinue_exit_loader(void) {
    jinue_message_t message;
    message.send_buffers        = NULL;
    message.send_buffers_length = 0;
    message.recv_buffers        = NULL;
    message.recv_buffers_length = 0;

    int errnum;
    uintptr_t errcode;

    int status = jinue_send(
        JINUE_DESC_LOADER_ENDPOINT,
        JINUE_MSG_EXIT,
        &message,
        &errnum,
        &errcode
    );

    if(status >= 0) {
        jinue_error("error: jinue_send() unexpectedly succeeded for JINUE_MSG_EXIT");
        return -1;
    }

    if(errnum == JINUE_EIO) {
        return 0;
    }

    if(errnum == JINUE_EPROTO) {
        jinue_error("error: loader set error code to: %s.", strerror(errcode));
    } else {
        jinue_error("error: jinue_send() failed: %s.", strerror(errnum));
    }

    return status;
}
