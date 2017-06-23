#include <stddef.h>
#include <slab.h>
#include <thread.h>

thread_t *current_thread;

slab_cache_t *thread_cache;


thread_t *create_thread(addr_t kernel_stack) {
    thread_t *thread;
    
    /* allocated first thread */
    thread = (thread_t *)slab_cache_alloc(thread_cache);
    
    /* initialize fields */
    thread->kernel_stack  = kernel_stack;
    thread->local_storage = NULL;
    thread->perrno        = NULL;
    
    return thread;
}

thread_t *create_initial_thread(addr_t kernel_stack) {
    thread_t *thread;
    
    /* create thread control block slab cache */
    thread_cache = slab_cache_create(
        "thread_cache",
        sizeof(thread_t),
        0,
        NULL,
        NULL,
        0 );
    
    /* allocate first thread */
    thread = create_thread(kernel_stack);
    
    return thread;
}
