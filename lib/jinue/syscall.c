#include <jinue/errno.h>
#include <jinue/ipc.h>
#include <jinue/syscall.h>
#include <stdbool.h>


typedef void (*syscall_stub_t)(jinue_syscall_args_t *args);

/* these are defined in stubs.asm */

void syscall_fast_intel(jinue_syscall_args_t *args);

void syscall_fast_amd(jinue_syscall_args_t *args);

void syscall_intr(jinue_syscall_args_t *args);


static syscall_stub_t syscall_stubs[] = {
        [SYSCALL_METHOD_FAST_INTEL] = syscall_fast_intel,
        [SYSCALL_METHOD_FAST_AMD]   = syscall_fast_amd,
        [SYSCALL_METHOD_INTR]       = syscall_intr
};

static char *syscall_stub_names[] = {
        [SYSCALL_METHOD_FAST_INTEL] = "SYSENTER/SYSEXIT (fast Intel)",
        [SYSCALL_METHOD_FAST_AMD]   = "SYSCALL/SYSRET (fast AMD)",
        [SYSCALL_METHOD_INTR]       = "interrupt",
};

static int syscall_stub_index           = SYSCALL_METHOD_INTR;

static syscall_stub_t syscall_stub_ptr  = syscall_intr;


void jinue_call_raw(jinue_syscall_args_t *args) {
	syscall_stub_ptr(args);
}

int jinue_call(jinue_syscall_args_t *args, int *perrno) {
	jinue_call_raw(args);

	if(perrno != NULL) {
		if(args->arg1 != 0) {
			*perrno = args->arg1;
		}
	}

	return (int)(args->arg0);
}

void jinue_get_syscall_implementation(void) {
    jinue_syscall_args_t args;

    args.arg0 = SYSCALL_FUNCT_SYSCALL_METHOD;
    args.arg1 = 0;
    args.arg2 = 0;
    args.arg3 = 0;

    jinue_call_raw(&args);

    syscall_stub_index  = jinue_get_return(&args);
    syscall_stub_ptr    = syscall_stubs[syscall_stub_index];
}

const char *jinue_get_syscall_implementation_name(void) {
    return syscall_stub_names[syscall_stub_index];
}

void jinue_set_thread_local_storage(void *addr, size_t size) {
	jinue_syscall_args_t args;

	args.arg0 = SYSCALL_FUNCT_SET_THREAD_LOCAL_ADDR;
	args.arg1 = (uintptr_t)addr;
	args.arg2 = (uintptr_t)size;
	args.arg3 = 0;

	(void)jinue_call(&args, NULL);
}

void *jinue_get_thread_local_storage(void) {
	jinue_syscall_args_t args;

	args.arg0 = SYSCALL_FUNCT_GET_THREAD_LOCAL_ADDR;
	args.arg1 = 0;
	args.arg2 = 0;
	args.arg3 = 0;

	(void)jinue_call(&args, NULL);

	return (void *)jinue_get_return_uintptr(&args);
}

int jinue_get_free_memory(memory_block_t *buffer, size_t buffer_size, int *perrno) {
	return jinue_send(
	        SYSCALL_FUNCT_GET_FREE_MEMORY,
	        -1,                 /* target */
	        (char *)buffer,     /* buffer */
	        buffer_size,        /* buffer size */
	        0,                  /* data size */
	        0,                  /* number of descriptors */
	        perrno);            /* perrno */
}

int jinue_thread_create(void (*entry)(), void *stack, int *perrno) {
    jinue_syscall_args_t args;

    args.arg0 = SYSCALL_FUNCT_THREAD_CREATE;
    args.arg1 = 0;
    args.arg2 = (uintptr_t)entry;
    args.arg3 = (uintptr_t)stack;

    return jinue_call(&args, perrno);
}

int jinue_yield(void) {
    jinue_syscall_args_t args;

    args.arg0 = SYSCALL_FUNCT_THREAD_YIELD;
    args.arg1 = false;
    args.arg2 = 0;
    args.arg3 = 0;

    return jinue_call(&args, NULL);
}

void jinue_thread_exit(void) {
    jinue_syscall_args_t args;

    args.arg0 = SYSCALL_FUNCT_THREAD_YIELD;
    args.arg1 = true;
    args.arg2 = 0;
    args.arg3 = 0;

    (void)jinue_call(&args, NULL);
}
