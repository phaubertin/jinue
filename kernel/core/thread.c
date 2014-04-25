#include <stddef.h>
#include <thread.h>

thread_t *current_thread;

void thread_init(thread_t *thread, addr_t kernel_stack) {
    thread->kernel_stack  = kernel_stack;
    thread->local_storage = NULL;
    thread->perrno        = NULL;
}
