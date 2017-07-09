#ifndef JINUE_KERNEL_OBJECT_H
#define JINUE_KERNEL_OBJECT_H

#include <stdint.h>

#define OBJECT_FLAG_NONE        0

#define OBJECT_FLAG_DESTROYED   (1<<0)


#define OBJECT_REF_FLAG_NONE    0

#define OBJECT_REF_FLAG_CLOSED  (1<<0)

#define OBJECT_REF_FLAG_OWNER   (1<<1)


#define OBJECT_TYPE_THREAD  1

#define OBJECT_TYPE_IPC     2


typedef struct {
    int type;
    int ref_count;
    int flags;
} object_header_t;

typedef struct {
    object_header_t *object;
    int              flags;
    uintptr_t        cookie;
} object_ref_t __attribute__ ((aligned (16)));

static inline void object_header_init(object_header_t *header, int type) {
    header->type        = type;
    header->ref_count   = 0;
    header->flags       = OBJECT_FLAG_NONE;
}

#endif
