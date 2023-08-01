/*
 * Copyright (C) 2019-2022 Philippe Aubertin.
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

#include <stdbool.h>
#include <stddef.h>

void *memset(void *s, int c, size_t n) {
    size_t   idx;
    char    *cs = s;

    for(idx = 0; idx < n; ++idx) {
        cs[idx] = c;
    }
    
    return s;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *uc1 = s1;
    const unsigned char *uc2 = s2;

    while(n > 0) {
        if(*uc1 > *uc2) {
            return 1;
        }
        if(*uc1 < *uc2) {
            return -1;
        }
        ++uc1;
        ++uc2;
        --n;
    }

    return 0;
}

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    size_t       idx;
    char        *cdest  = dest;
    const char  *csrc   = src;

    for(idx = 0; idx < n; ++idx) {
        cdest[idx] = csrc[idx];
    }

    return dest;
}

int strcmp(const char *s1, const char *s2) {
    while(true) {
        if(*s1 > *s2) {
            return 1;
        }
        if(*s1 < *s2) {
            return -1;
        }
        if(*s1 == '\0') {
            return 0;
        }
        ++s1;
        ++s2;
    }
}

char *strcpy(char *restrict dest, const char *restrict src) {
    size_t       idx;
    char        *cdest  = dest;
    const char  *csrc   = src;

    for(idx = 0; csrc[idx] != '\0'; ++idx) {
        cdest[idx] = csrc[idx];
    }

    return dest;
}

size_t strlen(const char *s) {
    size_t count = 0;

    while(*s != 0) {
        ++s;
        ++count;
    }
    
    return count;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while(n > 0) {
        if(*s1 > *s2) {
            return 1;
        }
        if(*s1 < *s2) {
            return -1;
        }
        if(*s1 == '\0') {
            return 0;
        }
        ++s1;
        ++s2;
        --n;
    }

    return 0;
}

size_t strnlen(const char *s, size_t maxlen) {
    size_t count = 0;

    while(count < maxlen && *s != 0) {
        ++s;
        ++count;
    }

    return count;
}
