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
#include <sys/mman.h>
#include <internals.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tar.h>
#include "../core/meminfo.h"
#include "../streams/stream.h"
#include "alloc.h"
#include "tar.h"

/** tar record size - must be 512 */
#define RECORD_SIZE             512

/** number of areas in the array used to allocate strings */
#define NUM_STRING_AREAS        4

/** PAX extended header - recognized but not supported */
#define FILETYPE_PAX            'x'

/** PAX global extended header - recognized but not supported */
#define FILETYPE_PAX_GLOBAL     'g'

/** GNU extension for long name */
#define FILETYPE_GNU_LONGNAME   'L'

/** GNU extension for long link name */
#define FILETYPE_GNU_LONGLINK   'K'


/**
 * Determine whether a character is an octal digit
 *
 * @param c character to check
 * @return true if octal digit, false otherwise
 *
 * */
static bool is_octal_digit(int c) {
    return c >= '0' && c <= '7';
}

/**
 * Get numerical value of octal digit
 *
 * @param c a character that is an octal digit
 * @return numerical value of octal digit
 *
 * */
static int octal_digit_value(int c) {
    return c - '0';
}

/**
 * Decode an octal field in a tar header block
 *
 * @param field pointer to field
 * @param length length of field
 * @return numerical value encoded in field
 *
 * */
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

/**
 * Check the checksum of a tar header block
 *
 * Compute the checksum over the header block and compare it against the
 * checksum field stored in that header block.
 *
 * @param header header block
 * @return true if checksum matches, false otherwise
 *
 * */
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

/**
 * Check ustar magic in a header block
 *
 * @param header header block
 * @return true for ustar format, false for an older format
 *
 * */
static bool is_ustar(const tar_header_t *header) {
    /* The specification states that the magic field should be NUL-terminated.
     * However, some implementations ("old GNU") put a space where the NUL
     * character should be. For better compatibility, we just check that the
     * "ustar" string (5 letters) is present and ignore what follows. */
    return strncmp(header->magic, TMAGIC, strlen(TMAGIC)) == 0;
}

/**
 * Check whether a RAM disk image is a tar archive
 *
 * This function reads from the stream, so the stream should be reset before
 * attempting to read the archive content.
 *
 * @param stream stream initialized on RAM disk image
 * @return true if image is a tar archive, false otherwise
 *
 * */
bool is_tar(stream_t *stream) {
    tar_header_t header;

    int status = stream_read(stream, &header, sizeof(header));

    if(status != STREAM_SUCCESS) {
        return false;
    }

    if(! is_checksum_valid(&header)) {
        return false;
    }

    if(is_ustar(&header)) {
        return header.name[0] != '\0' || header.prefix[0] != '\0';
    }

    return header.name[0] != '\0';
}

/** archive read state */
typedef struct {
    stream_t        *stream;
    alloc_area_t     dirent_area;
    alloc_area_t     string_areas[NUM_STRING_AREAS];
    unsigned char    buffer[RECORD_SIZE];
    /* Maximum path length is 256 characters, plus leading slash and NUL terminator. */
    char             filename[260];
    bool             at_end;
    bool             found_pax_extension;
} state_t;

/**
 * Get last tar header block read from archive
 *
 * @param state read state
 * @return header block
 *
 * */
const tar_header_t *state_header(const state_t *state) {
    return (const tar_header_t *)(state->buffer);
}

/**
 * Initialize read state
 *
 * @param state read state to initialize
 * @param stream stream from which to read archive content
 *
 * */
static jinue_dirent_t *initialize_state(state_t *state, stream_t *stream) {
    memset(state, 0, sizeof(state_t));
    state->stream = stream;
    return initialize_empty_dirent_list(&state->dirent_area);
}

/**
 * Check whether the last tar header block read from archive is archive trailer
 *
 * @param state read state
 * @return true if trailer found, false otherwise
 *
 * */
static bool found_trailer(state_t *state) {
    int total = 0;

    for(int idx = 0; idx < sizeof(state->buffer); ++idx) {
        total += state->buffer[idx];
    }

    return !total;
}

/**
 * Read the next header block from archive
 *
 * @param state read state
 * @return header block, NULL on failure
 *
 * */
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

    if(! is_checksum_valid(header)) {
        jinue_error("error: bad checksum in tar header");
        return NULL;
    }

    return header;
}

/**
 * Check whether a character is valid in a file name
 *
 * This function considers a slash character (/) as invalid; it is valid in a
 * file path as a separator but not in a file name (i.e. in a path component).
 *
 * @param c character to check
 * @return true if character is valid in a file name, false otherwise
 *
 * */
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
 * Sanitize a string that represents a complete or part of a file path.
 *
 * See description of parse_filename() for the sanitization rules.
 *
 * This function is called up to twice, i.e. once with the prefix field from the
 * header (ustar format) and once with the name string. It performs the actual
 * sanitization and copy for each of these strings individually.
 *
 * This function ensures the copied string starts with a slash (/), as long as
 * something remains after sanitization (i.e. this function can return zero and
 * write nothing):
 *
 *  - When passed the prefix string, or when passed the name string when there
 *    is no prefix string, this leading slash is the one that forms an absolute
 *    path.
 *  - When passed the name string when there is a prefix, this leading slash
 *    is the one that joins the prefix and name parts.
 *
 * @param output pointer to output buffer
 * @param input input string
 * @param length length of the input string field
 * @return output length excluding the NUL terminator, -1 on failure
 *
 * */
static int sanitize_filename_string(char *output, const char *input, size_t length) {
    /* The start of the string is the start of a path component, just as if we
     * just consumed a slash (/). */
    filename_state_t state = FILENAME_STATE_SLASH;

    const char *const start = output;
    int in_index = 0;

    while(in_index < length && input[in_index] != '\0') {
        /* We process one character from the input per loop iteration. */
        int c = input[in_index];

        if(c != '/' && !is_valid_filename_char(c)) {
            jinue_error("error: invalid character in file name/path");
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

            /* We haven't output the slash we encountered previously on the
             * input, so let's do this now. If we are at the beginning of
             * the string, then this slash is the leading slash. */
            *(output++) = '/';
            *(output++) = c;

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

            *(output++) = '/';
            *(output++) = '.';
            *(output++) = c;

            state = FILENAME_STATE_NAME;
            break;
        case FILENAME_STATE_DOT2:
            /* We just found a path component that starts with two dots. If
             * we now find a slash, then it means the whole path component is
             * ".." (i.e. parent directory) which we want to reject. */
            if(c == '/') {
                jinue_error("error: invalid or unsupported reference to parent directory (..) in file path");
                return -1;
            }

            *(output++) = '/';
            *(output++) = '.';
            *(output++) = '.';
            *(output++) = c;

            state = FILENAME_STATE_NAME;
            break;
        case FILENAME_STATE_NAME:
            if(c == '/') {
                state = FILENAME_STATE_SLASH;
                break;
            }

            *(output++) = c;

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
        jinue_error("error: invalid or unsupported reference to parent directory (..) in file path");
        return -1;
    }

    /* Add terminator, do not increment to ensure is it not included in length. */
    *output = '\0';

    return output - start;
}

/**
 * Sanitize file path from a tar header into state filename buffer
 *
 * We have to go a bit further than an typical tar implementation in terms of
 * path sanitization because we are not merely generating a path where we will
 * extract the file. Rather, we are generating the file's canonical path in a
 * virtual filesystem.
 *
 * Sanitization/conversion rules:
 * - Convert to an absolute path by prepending a slash (/) if there isn't one at the start.
 * - Strip off any trailing slash.
 * - Strip off any . path component.
 * - Strip off any empty path component (e.g. /foo//bar).
 * - Fail if there is any .. path component.
 * - Fail if any path component contains an unsupported character (see is_valid_filename_char()).
 *
 * @param state state with filename buffer
 * @param header header
 * @return file name, NULL on failure
 *
 * */
static const char *parse_filename(state_t *state, const tar_header_t *header) {
    int length;

    char *output = state->filename;

    if(is_ustar(header)) {
        length = sanitize_filename_string(output, header->prefix, sizeof(header->prefix));

        if(length < 0) {
            return NULL;
        }

        output += length;
    }

    length = sanitize_filename_string(output, header->name, sizeof(header->name));

    if(length < 0) {
        return NULL;
    }

    output += length;

    if(output - state->filename == 0) {
        if(header->typeflag != DIRTYPE) {
            jinue_error("error: empty file name/path");
            return NULL;
        }

        /* root directory */
        state->filename[0] = '/';
        state->filename[1] = '\0';
    }

    return state->filename;
}

/**
 * Copy a NUL-terminated string into newly allocated memory
 *
 * @param state read state (for allocator state)
 * @param str string to copy
 * @return string copy, NULL on failure
 *
 * */
static const char *copy_string(state_t *state, const char *str) {
    size_t length = strlen(str);

    char *newstr = allocate_from_areas_best_fit(state->string_areas, NUM_STRING_AREAS, length + 1);

    if(newstr == NULL) {
        return NULL;
    }

    strcpy(newstr, str);

    return newstr;
}

/**
 * Copy a string from a fixed-sized field into newly allocated memory
 *
 * The original string can be any length up to and including the specified field
 * length, which means it is not necessarily NUL-terminated). String copy is
 * NUL-terminated.
 *
 * @param state read state (for allocator state)
 * @param str address of string field
 * @param size length of string field
 * @return string copy, NULL on failure
 *
 * */
static char *copy_fixed_string(state_t *state, const char *str, size_t size) {
    size_t length = strnlen(str, size);

    char *newstr = allocate_from_areas_best_fit(state->string_areas, NUM_STRING_AREAS, length + 1);

    if(newstr == NULL) {
        return NULL;
    }

    memcpy(newstr, str, length);
    newstr[length] = '\0';

    return newstr;
}

/**
 * Read file data from archive into newly allocated memory
 *
 * Before calling this function, the (non-zero) size of the data must have been
 * set in the size member of the directory entry passed as argument.
 *
 * On success, this function sets the file member of the directory entry to the
 * newly allocated memory. The address and size of the newly allocated memory
 * are guaranteed to be page-aligned (the size is extended to the next page
 * boundary if needed). The bytes after the file data up to the end of the
 * allocated memory is cleared (i.e. zero padding).

 * @param state read state
 * @param dirent directory entry
 * @return zero (0) on success, other value on failure
 *
 * */
static int copy_file_data(state_t *state, jinue_dirent_t *dirent) {
    void *dest = allocate_page_aligned(dirent->size);

    if(dest == NULL) {
        jinue_error("error: could not allocate memory for file content");
        return -1;
    }

    size_t record_aligned_size = (dirent->size + RECORD_SIZE - 1) & ~((size_t)RECORD_SIZE - 1);

    int status = stream_read(state->stream, dest, record_aligned_size);

    if(status != STREAM_SUCCESS) {
        jinue_error("error: read error while reading file content");
        return -1;
    }

    size_t page_aligned_size = (dirent->size + JINUE_PAGE_SIZE - 1) & ~((size_t)JINUE_PAGE_SIZE - 1);

    /* clear padding */
    if(page_aligned_size > dirent->size) {
        memset((char *)dest + dirent->size, 0, page_aligned_size - dirent->size);
    }

    dirent->rel_value = (char *)dest - (char *)dirent;

    return 0;
}

/**
 * Map mode flags from tar definitions to Jinue definitions

 * @param tar_mode tar mode flags (decoded from octal field in header block)
 * @return mode flags conforming to Jinue definitions
 *
 * */
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

/**
 * Extract tar archive into a virtual filesystem
 *
 * @param stream stream from which to read the archive data
 * @return virtual filesystem root, NULL on failure
 *
 * */
const jinue_dirent_t *tar_extract(stream_t *stream) {
    state_t state;

    const uint64_t extracted_start = _libc_get_physmem_alloc_addr();
    
    jinue_dirent_t *root = initialize_state(&state, stream);

    if(root == NULL) {
        return NULL;
    }

    while(true) {
        const tar_header_t *header = extract_header(&state);

        if(header == NULL) {
            return NULL;
        }

        if(state.at_end) {
            break;
        }

        const char *filename = parse_filename(&state, header);

        if(filename == NULL) {
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
            jinue_error("error: tar archive with GNU long names extensions not supported");
            return NULL;
        default:
            jinue_warning("warning: file with unrecognized type treated as a regular file: %s", filename);
            type = JINUE_DIRENT_TYPE_FILE;
            break;
        }

        jinue_dirent_t *dirent = append_dirent_to_list(&state.dirent_area, type);

        if(dirent == NULL) {
            jinue_error("error: directory entry allocation failed");
            return NULL;
        }

        const char *name = copy_string(&state, filename);

        if(name == NULL) {
            jinue_error("error: failed to allocate memory for string (for file name)");
            return NULL;
        }

        dirent->rel_name    = name - (char *)dirent;
        dirent->size        = DECODE_OCTAL_FIELD(header->size);
        dirent->uid         = DECODE_OCTAL_FIELD(header->uid);
        dirent->gid         = DECODE_OCTAL_FIELD(header->gid);
        dirent->mode        = map_mode(DECODE_OCTAL_FIELD(header->mode));

        const char *link;

        switch(type) {
        case JINUE_DIRENT_TYPE_SYMLINK:
            link = copy_fixed_string(&state, header->linkname, sizeof(header->linkname));

            if(link == NULL) {
                jinue_error("error: failed to allocate memory for string (for symbolic link)");
                return NULL;
            }

            dirent->size        = 0;
            dirent->rel_value   = link - (char *)dirent;
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
    }

    const uint64_t extracted_end = _libc_get_physmem_alloc_addr();
    set_meminfo_ramdisk(extracted_start, extracted_end - extracted_start);

    return root;
}
