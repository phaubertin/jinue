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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tar.h>
#include "../streams/stream.h"
#include "alloc.h"
#include "tar.h"

/** tar record size - must be 512 */
#define RECORD_SIZE         512

/** number of areas in the array used to allocate strings */
#define NUM_STRING_AREAS    4

/** PAX extended header - recognized but not supported */
#define FILETYPE_PAX            'x'

/** PAX global extended header - recognized but not supported */
#define FILETYPE_PAX_GLOBAL     'g'

/** GNU extension for long name */
#define FILETYPE_GNU_LONGNAME   'L'

/** GNU extension for long link name */
#define FILETYPE_GNU_LONGLINK   'K'


static bool is_octal_digit(int c) {
    return c >= '0' && c <= '7';
}

static int octal_digit_value(int c) {
    return c - '0';
}

static int64_t decode_octal_field(const char *field, size_t length) {
    const char *const end = field + length;

    while(field < end && !is_octal_digit(*field)) {
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
    return strncmp(header->magic, TMAGIC, strlen(TMAGIC)) == 0;
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
        /* TODO check this */
        if(header->typeflag != DIRTYPE) {
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

static char *copy_symlink(alloc_area_t *string_areas, const tar_header_t *header) {
    size_t length = strnlen(header->linkname, sizeof(header->linkname));

    char *str = allocate_from_areas_best_fit(string_areas, NUM_STRING_AREAS, length + 1);

    if(str == NULL) {
        return NULL;
    }

    memcpy(str, header->linkname, length);
    str[length] = '\0';

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

static int copy_file_data(state_t *state, jinue_dirent_t *dirent) {
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
static void dump_dirent(const jinue_dirent_t* dirent) {
    char buffer[64];
    const char *type;

    switch(dirent->type) {
    case JINUE_DIRENT_TYPE_FILE:
        type = "file (regular)";
        break;
    case JINUE_DIRENT_TYPE_DIR:
        type = "directory";
        break;
    case JINUE_DIRENT_TYPE_SYMLINK:
        type = "symbolic link";
        break;
    case JINUE_DIRENT_TYPE_CHARDEV:
        type = "character device";
        break;
    case JINUE_DIRENT_TYPE_BLKDEV:
        type = "block device";
        break;
    case JINUE_DIRENT_TYPE_FIFO:
        type = "FIFO";
        break;
    default:
        snprintf(buffer, sizeof(buffer), "unknown (%i)", dirent->type);
        type = buffer;
        break;
    }

    jinue_info("directory entry:");
    jinue_info("    name:   %s", dirent->name);
    jinue_info("    type:   %s", type);
    jinue_info("    size:   %zu", dirent->size);
    jinue_info("    uid:    %i", dirent->uid);
    jinue_info("    gid:    %i", dirent->gid);
    jinue_info("    mode:   %o", (unsigned int)dirent->mode);
    jinue_info("    major:  %i", dirent->devmajor);
    jinue_info("    minor:  %i", dirent->devminor);

    jinue_info("    content:");

    if(dirent->file == NULL) {
        jinue_info("        (none)");
    }
    else {
        const unsigned char *raw = dirent->file;

        for(int idx = 0; idx < dirent->size; idx += 16) {
            if(idx == 64 && dirent->size > 256) {
                jinue_info("        ...");
                idx = (dirent->size - 64) & ~0xf;
            }

            jinue_info(
                    "        %8i %06x    %02hhx %02hhx %02hhx %02hhx  %02hhx %02hhx %02hhx %02hhx    %02hhx %02hhx %02hhx %02hhx  %02hhx %02hhx %02hhx %02hhx %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
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

    jinue_info("    link:");
    jinue_info("        %s", dirent->link == NULL ? "(none)" : dirent->link);
}

const int map_mode(int tar_mode) {
    const struct {int from; int to;} map[] = {
            {TSUID,     JINUE_ISUID},
            {TSGID,     JINUE_ISGID},
            {TUREAD,    JINUE_IRUSR},
            {TUWRITE,   JINUE_IWUSR},
            {TUEXEC,    JINUE_IXUSR},
            {TGREAD,    JINUE_IRGRP},
            {TGWRITE,   JINUE_IWGRP},
            {TGEXEC,    JINUE_IXGRP},
            {TOREAD,    JINUE_IROTH},
            {TOWRITE,   JINUE_IWOTH},
            {TOEXEC,    JINUE_IXOTH}
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

        /* TODO We should wait until we know we are going to extract the file
         * before we allocate the file path. We should stage the file path in
         * a fixed buffer in the mean time for use in warning/error messages. */
        const char *filename = copy_filename(state.string_areas, header);

        if(filename == NULL) {
            /* TODO error message on failure */
            return NULL;
        }

        int type;

        switch(header->typeflag) {
        case REGTYPE:
        case AREGTYPE:
        case CONTTYPE:
        {
            size_t name_length = strnlen(header->name, sizeof(header->name));

            if(!is_ustar(header) && name_length > 0 && header->name[name_length - 1] == '/') {
                type = JINUE_DIRENT_TYPE_DIR;
                break;
            }

            type = JINUE_DIRENT_TYPE_FILE;
        }
            break;
        case LNKTYPE:
            jinue_warning("hard links not supported, skipping %s", filename);
            continue;
        case SYMTYPE:
            type = JINUE_DIRENT_TYPE_SYMLINK;
            break;
        case CHRTYPE:
            type = JINUE_DIRENT_TYPE_CHARDEV;
            break;
        case BLKTYPE:
            type = JINUE_DIRENT_TYPE_BLKDEV;
            break;
        case DIRTYPE:
            type = JINUE_DIRENT_TYPE_DIR;
            break;
        case FIFOTYPE:
            type = JINUE_DIRENT_TYPE_FIFO;
            break;
        case FILETYPE_PAX:
        case FILETYPE_PAX_GLOBAL:
            jinue_error("error: PAX archive not supported");
            return NULL;
        case FILETYPE_GNU_LONGNAME:
        case FILETYPE_GNU_LONGLINK:
            jinue_error("error: tar archive with GNU long names extensions supported");
            return NULL;
            break;
        default:
            jinue_warning("warning: file with unrecognized type treated as a regular file: %s", filename);
            type = JINUE_DIRENT_TYPE_FILE;
            break;
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

        switch(type) {
        case JINUE_DIRENT_TYPE_SYMLINK:
            dirent->size    = 0;
            dirent->link    = copy_symlink(state.string_areas, header);

            if(dirent->link == NULL) {
                /* TODO error message on failure */
                return NULL;
            }

            break;
        case JINUE_DIRENT_TYPE_BLKDEV:
        case JINUE_DIRENT_TYPE_CHARDEV:
            dirent->size     = 0;
            dirent->devmajor = DECODE_OCTAL_FIELD(header->devmajor);
            dirent->devmajor = DECODE_OCTAL_FIELD(header->devminor);
            break;
        case JINUE_DIRENT_TYPE_DIR:
        case JINUE_DIRENT_TYPE_FIFO:
            dirent->size     = 0;
            break;
        }

        if(dirent->size > 0) {
            int status = copy_file_data(&state, dirent);

            if(status < 0) {
                return NULL;
            }
        }

        dump_dirent(dirent);
    }
}
