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

#include <jinue/loader.h>
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
