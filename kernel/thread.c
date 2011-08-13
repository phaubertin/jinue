#include <stddef.h>
#include <thread.h>

thread_t *current_thread;

void init_thread(thread_t *thread, addr_t stack) {
	thread->stack         = stack;
	thread->local_storage = NULL;
	thread->perrno        = NULL;
}
