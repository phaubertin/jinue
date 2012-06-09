#ifndef _JINUE_KERNEL_THREAD_H_
#define _JINUE_KERNEL_THREAD_H_

#include <jinue/types.h>


struct thread_t {
	addr_t	stack;
	addr_t  local_storage;
	size_t  local_storage_size;
	int    *perrno;
};

typedef struct thread_t thread_t;


extern thread_t *current_thread;


void thread_init(thread_t *thread, addr_t stack);

#endif

