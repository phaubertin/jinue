/*
 * Copyright (C) 2019 Philippe Aubertin.
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

#include <jinue/elf.h>
#include <jinue/errno.h>
#include <jinue/ipc.h>
#include <jinue/memory.h>
#include <jinue/syscall.h>
#include <jinue/types.h>
#include <jinue/vm.h>
#include <printk.h>
#include <stdint.h>
#include <stdlib.h>


#define THREAD_STACK_SIZE   4096

#define CALL_BUFFER_SIZE    512

int errno;

Elf32_auxv_t *auxvp;

int fd;

char thread_a_stack[THREAD_STACK_SIZE];


void thread_a(void) {
    int errno;
    char message[] = "Hello World!";

    if(fd < 0) {
        printk("Thread A has invalid descriptor.\n");
    }
    else {
        printk("Thread A got descriptor %u.\n", fd);

        printk("Thread A is sending message: %s\n", message);

        int ret = jinue_send(
                SYSCALL_FUNCT_USER_BASE,    /* function number */
                fd,                         /* target descriptor */
                message,                    /* buffer address */
                sizeof(message),            /* buffer size */
                sizeof(message),            /* data size */
                0,                          /* number of descriptors */
                &errno);                    /* error number */

        if(ret < 0) {
            printk("jinue_send() failed with error: %u.\n", errno);
        }
        else {
            printk("Thread A got reply from main thread: %s\n", message);
        }
    }

    printk("Thread A is exiting.\n");

    jinue_thread_exit();
}

static void dump_phys_memory_map(const jinue_mem_map_t *map) {
    unsigned int idx;

    printk("Dump of the BIOS memory map:\n");

    for(idx = 0; idx < map->num_entries; ++idx) {
        const jinue_mem_entry_t *entry = &map->entry[idx];

        printk(
                "%c [%q-%q] %s\n",
                (entry->type==E820_RAM)?'*':' ',
                entry->addr,
                entry->addr + entry->size - 1,
                jinue_pys_mem_type_description(entry->type)
        );
    }
}

int main(int argc, char *argv[], char *envp[]) {
    char call_buffer[CALL_BUFFER_SIZE];
    int status;
    
    /* say hello */
    printk("Process manager (%s) started.\n", argv[0]);

    /* get system call implementation so we can use something faster than the
     * interrupt-based one if available */
    jinue_get_syscall_implementation();
    
    printk("Using system call method '%s'.\n", jinue_get_syscall_implementation_name());
    
    /* get free memory blocks from microkernel */
    errno = 0;
    status = jinue_get_phys_memory((jinue_mem_map_t *)&call_buffer, sizeof(call_buffer), &errno);

    if(status != 0) {
        printk("error: could not get physical memory map from microkernel.\n");

        return EXIT_FAILURE;
    }

    dump_phys_memory_map((jinue_mem_map_t *)&call_buffer);

    printk("Creating IPC object descriptor.\n");

    int fd = jinue_create_ipc(JINUE_IPC_NONE, &errno);

    if(fd < 0) {
        printk("Error number: %u\n", errno);
    }
    else {
        char buffer[128];
        jinue_message_t message;

        printk("Main thread got descriptor %u.\n", fd);
        
        printk("Creating thread A.\n");

        (void)jinue_thread_create(thread_a, &thread_a_stack[THREAD_STACK_SIZE], NULL);

        int ret = jinue_receive(
                fd,
                buffer,
                sizeof(buffer),
                &message,
                &errno);

        if(ret < 0) {
            printk("jinue_receive() failed with error: %u.\n", errno);
        }
        else {
            char reply[] = "OK";

            printk("Main thread received message: %s\n", buffer);

            ret = jinue_reply(
                    reply,                      /* buffer address */
                    sizeof(reply),              /* buffer size */
                    sizeof(reply),              /* data size */
                    0,                          /* number of descriptors */
                    &errno);                    /* error number */

            if(ret < 0) {
                printk("jinue_reply() failed with error: %u.\n", errno);
            }
        }
    }

    printk("Main thread is running.\n");

    while (1) {
        (void)jinue_yield();
    }
    
    return EXIT_SUCCESS;
}
