#ifndef JINUE_KERNEL_OBJECT_DECL_H
#define JINUE_KERNEL_OBJECT_DECL_H

#include <stdint.h>

typedef struct {
    int type;
    int ref_count;
    int flags;
} object_header_t;

typedef struct {
    object_header_t *object;
    uintptr_t        flags;
    uintptr_t        cookie;
    uintptr_t        reserved;
} object_ref_t;

#endif
