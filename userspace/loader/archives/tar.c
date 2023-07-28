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
#include <jinue/util.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../streams/stream.h"
#include "alloc.h"
#include "tar.h"

/** tar record size - must be 512 */
#define RECORD_SIZE         512

/** number of areas in the array used to allocate strings */
#define NUM_STRING_AREAS    4

/** regular file */
#define FILETYPE_FILE       '0'

/** hard link */
#define FILETYPE_HARD_LINK  '1'

/** symbolic link */
#define FILETYPE_SYMLINK    '2'

/** character device */
#define FILETYPE_CHARDEV    '3'

/** block device */
#define FILETYPE_BLKDEV     '4'

/** directory */
#define FILETYPE_DIR        '5'

/** FIFO */
#define FILETYPE_FIFO       '6'

/** contiguous file - treated as a regular file */
#define FILETYPE_CONTFILE   '7'

/** PAX extended header - recognized but not supported */
#define FILETYPE_PAX        'x'

/** PAX global extended header - recognized but not supported */
#define FILETYPE_PAX_GLOBAL 'g'

/** set UID on execution */
#define TAR_ISUID 04000

/** set GID on execution */
#define TAR_ISGID 02000

/** read permission for file owner class */
#define TAR_IRUSR 00400

/** write permission for file owner class */
#define TAR_IWUSR 00200

/** execute/search permission for file owner class */
#define TAR_IXUSR 00100

/** read permission for file group class */
#define TAR_IRGRP 00040

/** write permission for file group class */
#define TAR_IWGRP 00020

/** execute/search permission for file group class */
#define TAR_IXGRP 00010

/** read permission for file other class */
#define TAR_IROTH 00004

/** write permission for file other class */
#define TAR_IWOTH 00002

/** execute/search permission for file other class */
#define TAR_IXOTH 00001


static bool is_octal_digit(int c) {
    return c >= '0' && c <= '7';
}

static int octal_digit_value(int c) {
    return c - '0';
}

static int64_t decode_octal_field(const char *field, size_t length) {
    const char *const end = field + length;

    while(field < end && *field == ' ') {
        ++field;
    }

    int64_t value = 0;

    while(field < end && is_octal_digit(*field)) {
        value <<= 3;
        value += octal_digit_value(*field);
        ++field;
    }

    return value;
}

#define DECODE_OCTAL_FIELD(x) decode_octal_field(x, sizeof(x))

static bool is_checksum_valid(const tar_header_t *header) {
    int checksum = ' ' * sizeof(header->chksum);

    for(int idx = 0; idx < sizeof(tar_header_t); ++idx) {
        checksum += ((unsigned char *)header)[idx];
    }

    for(int idx = 0; idx < sizeof(header->chksum); ++idx) {
        checksum -= header->chksum[idx];
    }

    return checksum == DECODE_OCTAL_FIELD(header->chksum);
}

static bool is_ustar(const tar_header_t *header) {
    const char *ustar = "ustar";
    return strncmp(header->magic, ustar, strlen(ustar)) == 0;
}

static int formatc(int c) {
    if(c < 32) {
        return ' ';
    }
    if(c > 126) {
        return ' ';
    }
    return c;
}

/* TODO remove this function */
static void dump_header(const tar_header_t *header) {
    const unsigned char *raw = (const unsigned char *)header;

    jinue_info("tar header:");

    for(int idx = 0; idx < 512; idx += 16) {
        jinue_info(
                "    %06i %04x    %02hhx %02hhx %02hhx %02hhx  %02hhx %02hhx %02hhx %02hhx    %02hhx %02hhx %02hhx %02hhx  %02hhx %02hhx %02hhx %02hhx %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
                idx,
                idx,
                raw[idx + 0],
                raw[idx + 1],
                raw[idx + 2],
                raw[idx + 3],
                raw[idx + 4],
                raw[idx + 5],
                raw[idx + 6],
                raw[idx + 7],
                raw[idx + 8],
                raw[idx + 9],
                raw[idx + 10],
                raw[idx + 11],
                raw[idx + 12],
                raw[idx + 13],
                raw[idx + 14],
                raw[idx + 15],
                formatc(raw[idx + 0]),
                formatc(raw[idx + 1]),
                formatc(raw[idx + 2]),
                formatc(raw[idx + 3]),
                formatc(raw[idx + 4]),
                formatc(raw[idx + 5]),
                formatc(raw[idx + 6]),
                formatc(raw[idx + 7]),
                formatc(raw[idx + 8]),
                formatc(raw[idx + 9]),
                formatc(raw[idx + 10]),
                formatc(raw[idx + 11]),
                formatc(raw[idx + 12]),
                formatc(raw[idx + 13]),
                formatc(raw[idx + 14]),
                formatc(raw[idx + 15])
        );
    }

}

bool is_tar(stream_t *stream) {
    tar_header_t header;

    int status = stream_read(stream, &header, sizeof(header));

    if(status != STREAM_SUCCESS) {
        return false;
    }

    dump_header(&header);

    if(! is_checksum_valid(&header)) {
        return false;
    }

    if(is_ustar(&header)) {
        return header.name[0] != '\0' || header.prefix[0] != '\0';
    }

    return header.name[0] != '\0';
}

static bool is_valid_filename_char(int c) {
    if(c < ' ') {
        /* Exclude control characters and NUL. */
        return false;
    }

    switch(c) {
    case '/':
    case ':':
    case '\\':
    case '*':
    case '?':
    case '|':
    case '"':
    case '<':
    case '>':
#define DEL 127
    case DEL:
#undef DEL
        return false;
    default:
        return true;
    }
}

typedef enum {
    FILENAME_STATE_SLASH,
    FILENAME_STATE_DOT1,
    FILENAME_STATE_DOT2,
    FILENAME_STATE_NAME
} filename_state_t;


/**
 * Sanitize and copy file path string.
 *
 * See filename_copy().
 *
 * This function is called up to twice, i.e. once with the prefix field from the
 * header (ustar format) and once with the name string. It performs the actual
 * sanitization and copy for each of these strings individually.
 *
 * This function ensures the copied string starts with a slash (/), as long as
 * something remains after sanitization (i.e. this function can return zero and
 * copy nothing):
 *
 *  - When called on the prefix string, or when called on the name string when
 *    there is no prefix string, this leading slash is the one used to form an
 *    absolute path.
 *  - When called on the name string when there is a prefix, this leading slash
 *    is the one that joins the prefix and name parts.
 *
 * This function does not NUL-terminate the string it copies (filename_copy()
 * does that).
 *
 * @param output pointer to output buffer, NULL to only determine output length
 * @param input input string
 * @param length length of the input string
 * @param header header
 * @return output length excluding NUL terminator (none written), -1 on failure
 *
 * */
static int copy_filename_string(char *output, const char *input, size_t length) {
    /* The start of the string is the start of a path component, just as if we
     * just consumed a slash (/). */
    filename_state_t state = FILENAME_STATE_SLASH;

    int in_index = 0;
    int out_count = 0;

    while(in_index < length && input[in_index] != '\0') {
        /* We process one character from the input per loop iteration. */
        int c = input[in_index];

        if(c != '/' && !is_valid_filename_char(c)) {
            jinue_error("error: ignoring file with invalid character in file name/path");
            return -1;
        }

        switch(state) {
        case FILENAME_STATE_SLASH:
            /* We are at the beginning of a path component, i.e. either we are
             * at the start of the string or the previous character we read was
             * a slash (/). If we find another slash, we can just discard it so
             * we don't end up with empty path components. That is, we interpret
             * any number of consecutive slashes as a single one. */
            if(c == '/') {
                break;
            }

            /* If the path component starts with a dot(.) let's wait and see if
             * its a ".", a ".." or just a file name that starts with a dot. */
            if(c == '.') {
                state = FILENAME_STATE_DOT1;
                break;
            }

            if(output != NULL) {
                /* We haven't output the slash we encountered previously on the
                 * input, so let's do this now. If we are at the beginning of
                 * the string, then this slash is the leading slash. */
                *(output++) = '/';
                *(output++) = c;
            }

            out_count += 2;

            state = FILENAME_STATE_NAME;
            break;
        case FILENAME_STATE_DOT1:
            /* The last character we found was a dot (.) and it was the first
             * character of a path component. If its followed by a slash, then
             * we can just consume it and not output anything, i.e. we just
             * strip single-dot components. */
            if(c == '/') {
                state = FILENAME_STATE_SLASH;
                break;
            }

            /* If we find a second dot, let's wait and see if it's a ".." (i.e.
             * parent directory) component or just a file name that starts with
             * two dots. */
            if(c == '.') {
                state = FILENAME_STATE_DOT2;
                break;
            }

            if(output != NULL) {
                *(output++) = '/';
                *(output++) = '.';
                *(output++) = c;
            }

            out_count += 3;

            state = FILENAME_STATE_NAME;
            break;
        case FILENAME_STATE_DOT2:
            /* We just found a path component that starts with two dots. If
             * we now find a slash, then it means the whole path component is
             * ".." (i.e. parent directory) which we want to reject. */
            if(c == '/') {
                jinue_error("error: ignoring file with reference to parent directory (..) in path");
                return -1;
            }

            if(output != NULL) {
                *(output++) = '/';
                *(output++) = '.';
                *(output++) = '.';
                *(output++) = c;
            }

            out_count += 4;

            state = FILENAME_STATE_NAME;
            break;
        case FILENAME_STATE_NAME:
            if(c == '/') {
                state = FILENAME_STATE_SLASH;
            }

            if(output != NULL) {
                *(output++) = c;
            }

            ++out_count;
            break;
        }

        ++in_index;
    }

    /* If the last character was a "normal" character (FILENAME_STATE_NAME state),
     * we already output everything we needed to. If the string ended with a
     * slash or with a "." component, we just strip that off. The only remaining
     * case to handle at the end of the string is the one where the path ends
     * with a ".." component (FILENAME_STATE_DOT2 state).  */
    if(state == FILENAME_STATE_DOT2) {
        jinue_error("error: ignoring file with reference to parent directory (..) in path");
        return -1;
    }

    return out_count;
}

/**
 * Sanitize file path from a tar header and copy it to an output string buffer.
 *
 * We have to go a bit further than an typical tar implementation in terms of
 * path sanitization because we are not merely generating a path where we will
 * extract the file. Rather, we are generating the file's canonical path in a
 * virtual filesystem.
 *
 * Sanitization rules:
 * - Convert to an absolute path by prepending a slash (/) if there isn't one at the start.
 * - Strip off any trailing slash.
 * - Strip off any . path component.
 * - Strip off any empty path component (e.g. /foo//bar).
 * - Fail if there is any .. path component.
 * - Fail if any path component contains an unsupported character (see is_valid_filename_char()).
 *
 * This function writes a file path of arbitrary length, with no size/length
 * argument to set a limit. Expected usage is to determine the length of output
 * by a first call where the output argument is set to NULL, allocate sufficient
 * space, then call it again for the actual copy.
 *
 * @param output pointer to output buffer, NULL to only determine output length
 * @param header header
 * @return path length including NUL terminator, -1 on failure
 *
 * */
static int copy_filename_to(char *output, const tar_header_t *header) {
    int prefix_length = 0;

    if(is_ustar(header)) {
        prefix_length = copy_filename_string(output, header->prefix, sizeof(header->prefix));

        if(prefix_length < 0) {
            return -1;
        }

        if(output != NULL) {
            output += prefix_length;
        }
    }

    int name_length = copy_filename_string(output, header->name, sizeof(header->name));

    if(name_length < 0) {
        return -1;
    }

    if(output != NULL) {
        output += name_length;
    }

    int total = prefix_length + name_length;

    if(total == 0) {
        if(header->typeflag != FILETYPE_DIR) {
            jinue_error("error: ignoring file with empty file name/path");
            return -1;
        }

        if(output != NULL) {
            /* root directory */
            *(output++) = '/';
        }

        ++total;
    }

    if(output != NULL) {
        /* NUL terminator */
        *(output++) = '\0';
    }

    ++total;

    return total;
}

static char *copy_filename(alloc_area_t *string_areas, const tar_header_t *header) {
    int size = copy_filename_to(NULL, header);

    if(size < 0) {
        return NULL;
    }

    char *str = allocate_from_areas_best_fit(string_areas, NUM_STRING_AREAS, size);

    if(str == NULL) {
        return NULL;
    }

    (void)copy_filename_to(str, header);

    return str;
}

typedef struct {
    stream_t        *stream;
    alloc_area_t     dirent_area;
    alloc_area_t     string_areas[NUM_STRING_AREAS];
    unsigned char    buffer[RECORD_SIZE];
    bool             at_end;
    bool             found_pax_extension;
} state_t;

const tar_header_t *state_header(const state_t *state) {
    return (const tar_header_t *)(state->buffer);
}

void initialize_state(state_t *state, stream_t *stream) {
    memset(state, 0, sizeof(state_t));
    initialize_empty_dirent_list(&state->dirent_area);
    state->stream = stream;
}

static bool found_trailer(state_t *state) {
    int total = 0;

    for(int idx = 0; idx < sizeof(state->buffer); ++idx) {
        total += state->buffer[idx];
    }

    return !total;
}

const tar_header_t *extract_header(state_t *state) {
    int status = stream_read(state->stream, state->buffer, sizeof(state->buffer));

    if(status != STREAM_SUCCESS) {
        jinue_error("error: read error while reading file header");
        return NULL;
    }

    const tar_header_t *header = state_header(state);

    if(found_trailer(state)) {
        state->at_end = true;
        return header;
    }

    dump_header(header);

    if(! is_checksum_valid(header)) {
        jinue_error("error: bad checksum in tar header.");
        return NULL;
    }

    return header;
}

static int copy_file(state_t *state, jinue_dirent_t *dirent) {
    if(dirent->size == 0) {
        /* Empty file, legitimate case, nothing to do. */
        return 0;
    }

    dirent->file = allocate_page_aligned(dirent->size);

    if(dirent->file == NULL) {
        jinue_error("error: could not allocate memory for file content");
        return -1;
    }

    size_t record_aligned_size = (dirent->size + RECORD_SIZE - 1) & ~((size_t)RECORD_SIZE - 1);

    int status = stream_read(state->stream, (void *)dirent->file, record_aligned_size);

    if(status != STREAM_SUCCESS) {
        jinue_error("error: read error while reading file content");
        return -1;
    }

    size_t page_aligned_size = (dirent->size + PAGE_SIZE - 1) & ~((size_t)PAGE_SIZE - 1);

    /* clear padding */
    if(page_aligned_size > dirent->size) {
        memset((char *)dirent->file + dirent->size, 0, page_aligned_size - dirent->size);
    }

    return 0;
}

/* TODO delete this function */
static void dump_dirent(const jinue_dirent_t*dirent) {
    jinue_info("directory entry:");
    jinue_info("    name:   %s", dirent->name);
    jinue_info("    type:   %i", dirent->type);
    jinue_info("    size:   %zu", dirent->size);
    jinue_info("    uid:    %i", dirent->uid);
    jinue_info("    gid:    %i", dirent->gid);
    jinue_info("    mode:   %o", (unsigned int)dirent->mode);
    jinue_info("    major:  %i", dirent->devmajor);
    jinue_info("    minor:  %i", dirent->devminor);

    if(dirent->file != NULL) {
        jinue_info("    content:");

        const unsigned char *raw = dirent->file;
        for(int idx = 0; idx < 64; idx += 16) {
            jinue_info(
                    "        %06i %04x    %02hhx %02hhx %02hhx %02hhx  %02hhx %02hhx %02hhx %02hhx    %02hhx %02hhx %02hhx %02hhx  %02hhx %02hhx %02hhx %02hhx %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
                    idx,
                    idx,
                    raw[idx + 0],
                    raw[idx + 1],
                    raw[idx + 2],
                    raw[idx + 3],
                    raw[idx + 4],
                    raw[idx + 5],
                    raw[idx + 6],
                    raw[idx + 7],
                    raw[idx + 8],
                    raw[idx + 9],
                    raw[idx + 10],
                    raw[idx + 11],
                    raw[idx + 12],
                    raw[idx + 13],
                    raw[idx + 14],
                    raw[idx + 15],
                    formatc(raw[idx + 0]),
                    formatc(raw[idx + 1]),
                    formatc(raw[idx + 2]),
                    formatc(raw[idx + 3]),
                    formatc(raw[idx + 4]),
                    formatc(raw[idx + 5]),
                    formatc(raw[idx + 6]),
                    formatc(raw[idx + 7]),
                    formatc(raw[idx + 8]),
                    formatc(raw[idx + 9]),
                    formatc(raw[idx + 10]),
                    formatc(raw[idx + 11]),
                    formatc(raw[idx + 12]),
                    formatc(raw[idx + 13]),
                    formatc(raw[idx + 14]),
                    formatc(raw[idx + 15])
            );
        }
    }
}

const int map_mode(int tar_mode) {
    const struct {int from; int to;} map[] = {
            {TAR_ISUID, JINUE_ISUID},
            {TAR_ISGID, JINUE_ISGID},
            {TAR_IRUSR, JINUE_IRUSR},
            {TAR_IWUSR, JINUE_IWUSR},
            {TAR_IXUSR, JINUE_IXUSR},
            {TAR_IRGRP, JINUE_IRGRP},
            {TAR_IWGRP, JINUE_IWGRP},
            {TAR_IXGRP, JINUE_IXGRP},
            {TAR_IROTH, JINUE_IROTH},
            {TAR_IWOTH, JINUE_IWOTH},
            {TAR_IXOTH, JINUE_IXOTH}
    };

    int mode = 0;

    for(int idx = 0; idx < sizeof(map) / sizeof(map[0]); ++idx) {
        if(tar_mode & map[idx].from) {
            mode |= map[idx].to;
        }
    }

    return mode;
}

const jinue_dirent_t *tar_extract(stream_t *stream) {
    state_t state;

    initialize_state(&state, stream);

    jinue_dirent_t *root = initialize_empty_dirent_list(&state.dirent_area);

    if(root == NULL) {
        return NULL;
    }

    while(true) {
        const tar_header_t *header = extract_header(&state);

        if(header == NULL) {
            return NULL;
        }

        if(state.at_end) {
            return root;
        }

        int type;
        size_t name_length;

        switch(header->typeflag) {
        case FILETYPE_FILE:
        case FILETYPE_CONTFILE:
            name_length = strnlen(header->name, sizeof(header->name));

            if(!is_ustar(header) && name_length > 0 && header->name[name_length - 1] == '/') {
                type = JINUE_DIRENT_TYPE_DIR;
                break;
            }

            type = JINUE_DIRENT_TYPE_FILE;
            break;
        case FILETYPE_HARD_LINK:
            /* TODO how should we handle hard links? */
            type = 0;
            break;
        case FILETYPE_SYMLINK:
            type = JINUE_DIRENT_TYPE_SYMLINK;
            break;
        case FILETYPE_CHARDEV:
            type = JINUE_DIRENT_TYPE_CHARDEV;
            break;
        case FILETYPE_BLKDEV:
            type = JINUE_DIRENT_TYPE_BLKDEV;
            break;
        case FILETYPE_DIR:
            type = JINUE_DIRENT_TYPE_DIR;
            break;
        case FILETYPE_FIFO:
            type = JINUE_DIRENT_TYPE_FIFO;
            break;
        case FILETYPE_PAX:
        case FILETYPE_PAX_GLOBAL:
            jinue_error("error: PAX archive not supported");
            return NULL;
        default:
            jinue_error("error: file type not supported");
            return NULL;
        }

        const char *filename = copy_filename(state.string_areas, header);

        if(filename == NULL) {
            /* TODO error message on failure */
            return NULL;
        }

        jinue_dirent_t *dirent = append_dirent_to_list(&state.dirent_area, type);

        if(dirent == NULL) {
            /* TODO error message on failure */
            return NULL;
        }

        dirent->name = filename;
        dirent->size = DECODE_OCTAL_FIELD(header->size);
        dirent->uid  = DECODE_OCTAL_FIELD(header->uid);
        dirent->gid  = DECODE_OCTAL_FIELD(header->gid);
        dirent->mode = map_mode(DECODE_OCTAL_FIELD(header->mode));

        int status;

        switch(type) {
        case JINUE_DIRENT_TYPE_FILE:
            status = copy_file(&state, dirent);

            if(status < 0) {
                return NULL;
            }
            break;
        case JINUE_DIRENT_TYPE_BLKDEV:
        case JINUE_DIRENT_TYPE_CHARDEV:
            dirent->devmajor = DECODE_OCTAL_FIELD(header->devmajor);
            dirent->devmajor = DECODE_OCTAL_FIELD(header->devminor);
            break;
        }

        dump_dirent(dirent);
    }
}
