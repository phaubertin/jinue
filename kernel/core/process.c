#include <process.h>
#include <object.h>
#include <slab.h>
#include <stddef.h>
#include <string.h>


static slab_cache_t *process_cache;

static void process_ctor(void *buffer, size_t ignore) {
    process_t *process = buffer;

    object_header_init(&process->header, OBJECT_TYPE_PROCESS);
}

void process_boot_init(void) {
    process_cache = slab_cache_create(
            "process_cache",
            sizeof(process_t),
            0,
            process_ctor,
            NULL,
            SLAB_DEFAULTS );
}

process_t *process_create(void) {
    process_t *process = slab_cache_alloc(process_cache);

    if(process != NULL) {
        vm_create_addr_space(&process->addr_space);
        memset(&process->descriptors, 0, sizeof(process->descriptors));
    }

    return process;
}

object_ref_t *process_get_descriptor(process_t *process, int fd) {
    if(fd < 0 || fd > PROCESS_MAX_DESCRIPTORS) {
        return NULL;
    }

    return &process->descriptors[fd];
}

int process_unused_descriptor(process_t *process) {
    int idx;

    for(idx = 0; idx < PROCESS_MAX_DESCRIPTORS; ++idx) {
        object_ref_t *ref = process_get_descriptor(process, idx);

        if(! object_ref_is_valid(ref)) {
            return idx;
        }
    }

    return -1;
}
