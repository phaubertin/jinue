#include <jinue/syscall.h>
#include <stddef.h>

void console_printn(const char *message, unsigned int n) {
	const char *ptr = message;

    while(n > 0) {
    	unsigned int size = n;

    	if(size > JINUE_SEND_MAX_SIZE) {
    		size = JINUE_SEND_MAX_SIZE;
    	}

    	(void)jinue_send(
    	        SYSCALL_FUNCT_CONSOLE_PUTS,
    	        -1,             /* target */
    	        (char *)ptr,    /* buffer */
    	        size,           /* buffer size */
    	        size,           /* data size */
    	        0,              /* number of descriptors */
    	        NULL);          /* perrno */

    	ptr += size;
    	n   -= size;
    }
}

void console_putc(char c) {
	jinue_syscall_args_t args;

	args.arg0 = SYSCALL_FUNCT_CONSOLE_PUTC;
	args.arg1 = (uintptr_t)c;
	args.arg2 = 0;
	args.arg3 = 0;

	(void)jinue_call(&args, NULL);
}

void console_print(const char *message) {
    size_t count;

    count = 0;

    while(message[count] != 0) {
        ++count;
    }

    console_printn(message, count);
}
