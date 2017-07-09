#ifndef JINUE_KERNEL_THREAD_DECL_H
#define JINUE_KERNEL_THREAD_DECL_H

#include <jinue/list.h>
#include <hal/thread_ctx.h>
#include <object.h>

struct thread_t {
    object_header_t     header;
    thread_context_t    thread_ctx;
    jinue_node_t        thread_list;
};

typedef struct thread_t thread_t;

#endif
