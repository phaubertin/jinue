#ifndef _JINUE_KERNEL_THREAD_H_
#define _JINUE_KERNEL_THREAD_H_

#include <jinue/types.h>
#include <hal/vm.h>

struct thread_t {
	addr_t  stack;
	addr_t  local_storage;
	size_t  local_storage_size;
	int    *perrno;
};

typedef struct thread_t thread_t;


extern thread_t *current_thread;


void thread_init(thread_t *thread, addr_t stack);

void thread_start(addr_t entry, addr_t stack);

int thread_switch(addr_t vstack, pfaddr_t pstack, unsigned int flags, pte_t *pte, int next);

#endif

