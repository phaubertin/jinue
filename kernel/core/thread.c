#include <jinue-common/list.h>
#include <hal/thread.h>
#include <object.h>
#include <panic.h>
#include <thread.h>


static jinue_list_t ready_list = JINUE_LIST_STATIC;

thread_t *thread_create(
        process_t       *process,
        addr_t           entry,
        addr_t           user_stack) {
            
    thread_t *thread = thread_page_create(entry, user_stack);
    
    if(thread != NULL) {
        object_header_init(&thread->header, OBJECT_TYPE_THREAD);

        jinue_node_init(&thread->thread_list);
        
        thread->process     = process;
        thread->sender      = NULL;

        thread_ready(thread);
    }
    
    return thread;
}

void thread_ready(thread_t *thread) {
    /* add thread to the tail of the ready list to give other threads a chance
     * to run */
    jinue_list_enqueue(&ready_list, &thread->thread_list);
}

void thread_switch(
        thread_t *from_thread,
        thread_t *to_thread,
        bool blocked,
        bool do_destroy) {

    if(to_thread != from_thread) {
        thread_context_t    *from_context;
        process_t           *from_process;

        if(from_thread == NULL) {
            from_context = NULL;
            from_process = NULL;
        }
        else {
            from_context = &from_thread->thread_ctx;
            from_process = from_thread->process;

            /* Put the the thread we are switching away from (the current thread)
             * back into the ready list, unless it just blocked or it is being
             * destroyed. */
            if(! (do_destroy || blocked)) {
                thread_ready(from_thread);
            }
        }

        if(from_process != to_thread->process) {
            vm_switch_addr_space(&to_thread->process->addr_space);
        }

        thread_context_switch(
            from_context,
            &to_thread->thread_ctx,
            do_destroy);
    }
}

static thread_t *reschedule(thread_t *from_thread, bool from_can_run) {
    thread_t *to_thread = jinue_node_entry(
            jinue_list_dequeue(&ready_list),
            thread_t,
            thread_list );
    
    if(to_thread == NULL) {
        if(from_thread != NULL && from_can_run) {
            /* We just let the current thread run because there are no other
             * threads to run. */
            return from_thread;
        }
        else {
            /* Currently, scheduling is purely cooperative and only one CPU is
             * supported (so, there are no threads currently running on other
             * CPUs). What this means is that, once there are no more threads
             * running or ready to run, this situation will never change. */
            panic("No more thread to schedule");
        }
    }

    return to_thread;
}

void thread_yield_from(thread_t *from_thread, bool blocked, bool do_destroy) {
    bool from_can_run = ! (blocked || do_destroy);
    
    thread_switch(
            from_thread,
            reschedule(from_thread, from_can_run),
            blocked,
            do_destroy);
}
