#include <stddef.h>

void *memset(void *s, int c, size_t n) {
    size_t   idx;
    char    *cs = s;

    for(idx = 0; idx < n; ++idx) {
        cs[idx] = c;
    }
    
    return s;
}

void *memcpy(void *dest, const void *src, size_t n) {
    size_t       idx;
    char        *cdest  = dest;
    const char  *csrc   = src;

    for(idx = 0; idx < n; ++idx) {
        cdest[idx] = csrc[idx];
    }

    return dest;
}
