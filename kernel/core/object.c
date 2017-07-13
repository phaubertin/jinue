#include <object.h>
#include <thread.h>


object_ref_t *get_descriptor(thread_t *thread, int n) {
    return &thread->descriptors[n];
}

int find_unused_descriptor(thread_t *thread) {
    int idx;
    
    for(idx = 0; idx < THREAD_MAX_DESCRIPTORS; ++idx) {
        object_ref_t *ref = get_descriptor(thread, idx);
        
        if(! object_ref_is_valid(ref)) {
            return idx;
        }
    }
    
    return -1;
}
