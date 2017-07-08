#include <jinue/list.h>
#include <hal/thread.h>
#include <panic.h>
#include <thread.h>

static jinue_list_t ready_list = JINUE_LIST_STATIC;

thread_t *thread_create(
        addr_space_t    *addr_space,
        addr_t           entry,
        addr_t           user_stack) {
            
    thread_t *thread = thread_page_create(addr_space, entry, user_stack);
    
    if(thread != NULL) {
        jinue_node_init(&thread->thread_list);
        
        /* add the the head so it is scheduled next */
        jinue_list_push(&ready_list, &thread->thread_list);
    }
    
    return thread;
}

void thread_yield_from(thread_t *from_thread, bool do_destroy) {
    thread_t *to_thread = jinue_node_entry(
            jinue_list_dequeue(&ready_list),
            thread_t,
            thread_list );
    
    if(to_thread == NULL) {
        if(from_thread == NULL) {
            /* Currently, scheduling is purely cooperative and only one CPU is
             * supported (so, there are no threads currently running on other
             * CPUs). What this means is that, once there are no more threads
             * running or ready to run, this situation will never change. */
            panic("No more threads to schedule");
        }
        else {
            /* We just let the current thread run because there are no others to
             * yield to. */
        }
    }
    else {
        thread_context_t *from_context;
        
        if(from_thread == NULL) {
            from_context = NULL;
        }
        else {
            from_context = &from_thread->thread_ctx;
            
            if(! do_destroy) {
                /* add thread to the tail of the ready list so other threads run
                 * first */
                jinue_list_enqueue(&ready_list, &from_thread->thread_list);
            }
        }

        thread_context_switch(
            from_context,
            &to_thread->thread_ctx,
            do_destroy);
    }
}
