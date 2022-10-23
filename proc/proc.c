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

#include <sys/auxv.h>
#include <sys/elf.h>
#include <jinue/errno.h>
#include <jinue/ipc.h>
#include <jinue/memory.h>
#include <jinue/syscall.h>
#include <jinue/types.h>
#include <jinue/vm.h>
#include <printk.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#define THREAD_STACK_SIZE   4096

#define CALL_BUFFER_SIZE    512

#define MSG_FUNC_TEST       (SYSCALL_USER_BASE + 42)

static int errno;

static int fd;

static char ipc_test_thread_stack[THREAD_STACK_SIZE];

extern char **environ;

extern const Elf32_auxv_t *_jinue_libc_auxv;

static bool bool_getenv(const char *name) {
    const char *value = getenv(name);

    if(value == NULL) {
        return false;
    }

    return  strcmp(value, "enable") == 0 ||
            strcmp(value, "true") == 0 ||
            strcmp(value, "yes") == 0 ||
            strcmp(value, "1") == 0;
}

static void dump_phys_memory_map(const jinue_mem_map_t *map) {
    const char *name    = "DEBUG_DUMP_MEMORY_MAP";
    const char *value   = getenv(name);

    if(value == NULL) {
        return;
    }

    bool ram_only = true;

    if(strcmp(value, "all") == 0) {
        ram_only = false;
    }
    else if(! bool_getenv(name)) {
        return;
    }

    printk("Dump of the BIOS memory map%s:\n", ram_only?" (showing only available entries)":"");

    for(int idx = 0; idx < map->num_entries; ++idx) {
        const jinue_mem_entry_t *entry = &map->entry[idx];

        if(entry->type == E820_RAM || !ram_only) {
            printk(
                    "  %c [%q-%q] %s\n",
                    (entry->type==E820_RAM)?'*':' ',
                    entry->addr,
                    entry->addr + entry->size - 1,
                    jinue_phys_mem_type_description(entry->type)
            );
        }
    }
}

static void dump_cmdline_arguments(int argc, char *argv[]) {
    if(! bool_getenv("DEBUG_DUMP_CMDLINE_ARGS")) {
        return;
    }

    printk("Command line arguments:\n");

    for(int idx = 0; idx < argc; ++idx) {
        printk("  %s\n", argv[idx]);
    }
}

static void dump_environ(void) {
    if(! bool_getenv("DEBUG_DUMP_ENVIRON")) {
        return;
    }

    printk("Environment variables:\n");

    for(char **envvar = environ; *envvar != NULL; ++envvar) {
        printk("  %s\n", *envvar);
    }
}

static const char *auxv_type_name(int type) {
    const struct {
        const char  *name;
        int          type;
    } *entry, mapping[] = {
            {"AT_NULL",             JINUE_AT_NULL},
            {"AT_IGNORE",           JINUE_AT_IGNORE},
            {"AT_EXECFD",           JINUE_AT_EXECFD},
            {"AT_PHDR",             JINUE_AT_PHDR},
            {"AT_PHENT",            JINUE_AT_PHENT},
            {"AT_PHNUM",            JINUE_AT_PHNUM},
            {"AT_PAGESZ",           JINUE_AT_PAGESZ},
            {"AT_BASE",             JINUE_AT_BASE},
            {"AT_FLAGS",            JINUE_AT_FLAGS},
            {"AT_ENTRY",            JINUE_AT_ENTRY},
            {"AT_STACKBASE",        JINUE_AT_STACKBASE},
            {"JINUE_AT_HOWSYSCALL", JINUE_AT_HOWSYSCALL},
            {NULL, 0}
    };

    for(entry = mapping; entry->name != NULL; ++entry) {
        if(entry->type == type) {
            return entry->name;
        }
    }

    return NULL;
}

static const char *syscall_mechanism_name(int mechanism) {
    const char *names[] = {
            [SYSCALL_METHOD_INTR]       = "interrupt",
            [SYSCALL_METHOD_FAST_AMD]   = "SYSCALL/SYSRET (fast AMD)",
            [SYSCALL_METHOD_FAST_INTEL] = "SYSENTER/SYSEXIT (fast Intel)"
    };

    if(mechanism < 0 || mechanism > SYSCALL_METHOD_LAST) {
        return "?";
    }

    return names[mechanism];
}

static void dump_auxvec(void) {
    if(! bool_getenv("DEBUG_DUMP_AUXV")) {
        return;
    }

    printk("Auxiliary vectors:\n");

    for(const Elf32_auxv_t *entry = _jinue_libc_auxv; entry->a_type != JINUE_AT_NULL; ++entry) {
        const char *name = auxv_type_name(entry->a_type);

        if(name != NULL) {
            printk("  %s: %u/0x%x\n", name, entry->a_un.a_val, entry->a_un.a_val);
        }
        else {
            printk("  (%u): %u/0x%x\n", entry->a_type, entry->a_un.a_val, entry->a_un.a_val);
        }
    }
}

static void ipc_test_run_client(void) {
    /* The order of these buffers is shuffled on purpose because they will be
     * concatenated later and we don't want things to look OK by coincidence. */
    char reply2[5];
    char reply1[6];
    char reply3[40];
    int errno;

    if(fd < 0) {
        printk("Client thread has invalid descriptor.\n");
        return;
    }

    const char hello[] = "Hello ";
    const char world[] = "World!";

    printk("Client thread got descriptor %u.\n", fd);
    printk("Client thread is sending message.\n");

    jinue_const_buffer_t send_buffers[2];
    send_buffers[0].addr = hello;
    send_buffers[0].size = sizeof(hello) - 1;   /* do not include NUL terminator */
    send_buffers[1].addr = world;
    send_buffers[1].size = sizeof(world);       /* includes NUL terminator */

    jinue_buffer_t reply_buffers[3];
    reply_buffers[0].addr = reply1;
    reply_buffers[0].size = sizeof(reply1) - 1; /* minus one chunk is NUL terminated */
    reply_buffers[1].addr = reply2;
    reply_buffers[1].size = sizeof(reply2) - 1; /* minus one chunk is NUL terminated */
    reply_buffers[2].addr = reply3;
    reply_buffers[2].size = sizeof(reply3);     /* final NUL is part of the reply message */

    memset(reply1, 0, sizeof(reply1));
    memset(reply2, 0, sizeof(reply2));
    memset(reply3, 0, sizeof(reply3));

    jinue_message_t message;
    message.send_buffers        = send_buffers;
    message.send_buffers_length = 2;
    message.recv_buffers        = reply_buffers;
    message.recv_buffers_length = 3;

    intptr_t ret = jinue_send(fd, MSG_FUNC_TEST, &message, &errno);

    if(ret < 0) {
        printk("jinue_send() failed with error: %u.\n", errno);
        return;
    }

    printk("Client thread got reply from main thread:\n");
    printk("  data:             \"%s%s%s\"\n", reply1, reply2, reply3);
    printk("  size:             %u\n", ret);
}

static void ipc_test_client_thread(void) {
    ipc_test_run_client();

    printk("Client thread is exiting.\n");

    jinue_exit_thread();
}

static void run_ipc_test(void) {
    char recv_data[64];

    if(! bool_getenv("RUN_TEST_IPC")) {
        return;
    }

    printk("Running threading and IPC test...\n");

    int fd = jinue_create_ipc(JINUE_IPC_NONE, &errno);

    if(fd < 0) {
        printk("Creating IPC object descriptor.\n");
        printk("Error number: %u\n", errno);
        return;
    }

    printk("Main thread got descriptor %u.\n", fd);

    jinue_create_thread(ipc_test_client_thread, &ipc_test_thread_stack[THREAD_STACK_SIZE], NULL);

    jinue_buffer_t recv_buffer;
    recv_buffer.addr = recv_data;
    recv_buffer.size = sizeof(recv_data);

    jinue_message_t message;
    message.recv_buffers        = &recv_buffer;
    message.recv_buffers_length = 1;

    intptr_t ret = jinue_receive(fd, &message, &errno);

    if(ret < 0) {
        printk("jinue_receive() failed with error: %u.\n", errno);
        return;
    }

    int function = message.recv_function;

    if(function != MSG_FUNC_TEST) {
        printk("jinue_receive() unexpected function number: %u.\n", function);
        return;
    }

    printk("Main thread received message:\n");
    printk("  data:             \"%s\"\n", recv_data);
    printk("  size:             %u\n", ret);
    printk("  function:         %u (user base + %u)\n", function, function - SYSCALL_USER_BASE);
    printk("  cookie:           %u\n", message.recv_cookie);
    printk("  reply max. size:  %u\n", message.reply_max_size);

    const char reply_string[] = "Hi, Main Thread!";

    jinue_const_buffer_t reply_buffer;
    reply_buffer.addr = reply_string;
    reply_buffer.size = sizeof(reply_string);   /* includes NUL terminator */

    jinue_message_t reply;
    reply.send_buffers          = &reply_buffer;
    reply.send_buffers_length   = 1;

    ret = jinue_reply(&reply, &errno);

    if(ret < 0) {
        printk("jinue_reply() failed with error: %u.\n", errno);
    }

    printk("Main thread is running.\n");
}

int main(int argc, char *argv[]) {
    char call_buffer[CALL_BUFFER_SIZE];
    int status;

    /* Say hello.
     *
     * We shouldn't do this before the call to jinue_set_syscall_mechanism(). It
     * works because the system call implementation selected before the call is
     * made is the interrupt-based one, which is slow but always available.
     *
     * TODO once things stabilize, ensure jinue_syscall() actually fails if no
     * system call implementation was set. */
    printk("Jinue user space loader (%s) started.\n", argv[0]);

    dump_cmdline_arguments(argc, argv);
    dump_environ();
    dump_auxvec();

    /* Get system call mechanism/implementation from auxiliary vectors so we can
     * use something faster than the interrupt-based one if available and ensure
     * the one we attempt to use is supported. */
    errno           = 0;
    int mechanism   = getauxval(JINUE_AT_HOWSYSCALL);
    int ret         = jinue_set_syscall_mechanism(mechanism, &errno);

    if (ret < 0) {
        printk("Could not set system call mechanism: %i", errno);
    }

    printk("Using system call method '%s'.\n", syscall_mechanism_name(mechanism));

    /* get free memory blocks from microkernel */
    status = jinue_get_user_memory((jinue_mem_map_t *)&call_buffer, sizeof(call_buffer), &errno);

    if(status != 0) {
        printk("error: could not get physical memory map from microkernel.\n");

        return EXIT_FAILURE;
    }

    dump_phys_memory_map((jinue_mem_map_t *)&call_buffer);

    run_ipc_test();

    while (1) {
        jinue_yield_thread();
    }
    
    return EXIT_SUCCESS;
}
