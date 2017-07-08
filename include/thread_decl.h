#ifndef _JINUE_KERNEL_THREAD_DECL_H_
#define _JINUE_KERNEL_THREAD_DECL_H_

#include <jinue/list.h>
#include <hal/thread_ctx.h>

struct thread_t {
    /* the thread context switching code makes the assumption that the thread
     * context is the first member of this structure. */
    thread_context_t    thread_ctx;
    jinue_node_t        thread_list;
};

typedef struct thread_t thread_t;

#endif
