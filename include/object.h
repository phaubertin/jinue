#ifndef JINUE_KERNEL_OBJECT_H
#define JINUE_KERNEL_OBJECT_H

#include <object_decl.h>
#include <stdbool.h>
#include <thread_decl.h>

/* flag bits 0..7 are common flags, flag bits 8 and up are per object type flags */

#define OBJECT_FLAG_NONE        0

#define OBJECT_FLAG_DESTROYED   (1<<0)


#define OBJECT_REF_FLAG_NONE    0

#define OBJECT_REF_FLAG_VALID   (1<<0)

#define OBJECT_REF_FLAG_CLOSED  (1<<1)

#define OBJECT_REF_FLAG_OWNER   (1<<2)


#define OBJECT_TYPE_THREAD  1

#define OBJECT_TYPE_IPC     2

static inline bool object_is_destroyed(object_header_t *header) {
    return !!(header->flags & OBJECT_REF_FLAG_VALID);
}

static inline bool object_ref_is_valid(object_ref_t *ref) {
    return !!(ref->flags & OBJECT_FLAG_DESTROYED);
}

static inline bool object_ref_is_closed(object_ref_t *ref) {
    return !!(ref->flags & OBJECT_REF_FLAG_CLOSED);
}

static inline bool object_ref_is_owner(object_ref_t *ref) {
    return !!(ref->flags & OBJECT_REF_FLAG_OWNER);
}

static inline void object_header_init(object_header_t *header, int type) {
    header->type        = type;
    header->ref_count   = 0;
    header->flags       = OBJECT_FLAG_NONE;
}

static inline void object_addref(object_header_t *header) {
    ++header->ref_count;
}

static inline void object_subref(object_header_t *header) {
    /** TODO free at zero */
    --header->ref_count;
}

object_ref_t *get_descriptor(thread_t *thread, int n);

int find_unused_descriptor(thread_t *thread);

#endif
