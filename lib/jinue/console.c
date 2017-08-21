/*
 * Copyright (C) 2017 Philippe Aubertin.
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the author nor the names of other contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <jinue/ipc.h>
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
