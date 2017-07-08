#include <hal/thread.h>
#include <thread.h>

thread_t *thread_create(
        addr_space_t    *addr_space,
        addr_t           entry,
        addr_t           user_stack) {
            
    thread_t *thread = thread_page_create(addr_space, entry, user_stack);
    
    if(thread != NULL) {
        jinue_node_init(&thread->thread_list);
    }
    
    return thread;
}
