/*
 * Copyright (C) 2019-2022 Philippe Aubertin.
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
#include <jinue/logging.h>
#include <jinue/syscall.h>
#include <jinue/types.h>
#include <jinue/vm.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>


#define THREAD_STACK_SIZE   4096

#define CALL_BUFFER_SIZE    512

#define MSG_FUNC_TEST       (JINUE_SYS_USER_BASE + 42)

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

static const char *jinue_phys_mem_type_description(uint32_t type) {
    switch(type) {

    case JINUE_E820_RAM:
        return "Available";

    case JINUE_E820_RESERVED:
        return "Unavailable/Reserved";

    case JINUE_E820_ACPI:
        return "Unavailable/ACPI";

    default:
        return "Unavailable/Other";
    }
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

    jinue_info("Dump of the BIOS memory map%s:", ram_only?" (showing only available entries)":"");

    for(int idx = 0; idx < map->num_entries; ++idx) {
        const jinue_mem_entry_t *entry = &map->entry[idx];

        if(entry->type == JINUE_E820_RAM || !ram_only) {
            jinue_info(
                    "  %c [%016" PRIx64 "-%016" PRIx64 "] %s",
                    (entry->type==JINUE_E820_RAM)?'*':' ',
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

    jinue_info("Command line arguments:");

    for(int idx = 0; idx < argc; ++idx) {
        jinue_info("  %s", argv[idx]);
    }
}

static void dump_environ(void) {
    if(! bool_getenv("DEBUG_DUMP_ENVIRON")) {
        return;
    }

    jinue_info("Environment variables:");

    for(char **envvar = environ; *envvar != NULL; ++envvar) {
        jinue_info("  %s", *envvar);
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

static const char *syscall_implementation_name(int implementation) {
    const char *names[] = {
            [JINUE_SYSCALL_IMPL_INTERRUPT]         = "interrupt",
            [JINUE_SYSCALL_IMPL_FAST_AMD]     = "SYSCALL/SYSRET (fast AMD)",
            [JINUE_SYSCALL_IMPL_FAST_INTEL]   = "SYSENTER/SYSEXIT (fast Intel)"
    };

    if(implementation < 0 || implementation > JINUE_SYSCALL_IMPL_LAST) {
        return "?";
    }

    return names[implementation];
}

static void dump_auxvec(void) {
    if(! bool_getenv("DEBUG_DUMP_AUXV")) {
        return;
    }

    jinue_info("Auxiliary vectors:");

    for(const Elf32_auxv_t *entry = _jinue_libc_auxv; entry->a_type != JINUE_AT_NULL; ++entry) {
        const char *name = auxv_type_name(entry->a_type);

        if(name != NULL) {
            jinue_info("  %s: %u/0x%" PRIx32, name, entry->a_un.a_val, entry->a_un.a_val);
        }
        else {
            jinue_info("  (%u): %u/0x%" PRIx32, entry->a_type, entry->a_un.a_val, entry->a_un.a_val);
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
        jinue_error("Client thread has invalid descriptor.");
        return;
    }

    const char hello[] = "Hello ";
    const char world[] = "World!";

    jinue_info("Client thread got descriptor %i.", fd);
    jinue_info("Client thread is sending message.");

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
        jinue_error("jinue_send() failed with error: %i.", errno);
        return;
    }

    jinue_info("Client thread got reply from main thread:");
    jinue_info("  data:             \"%s%s%s\"", reply1, reply2, reply3);
    jinue_info("  size:             %" PRIuPTR, ret);
}

static void ipc_test_client_thread(void) {
    ipc_test_run_client();

    jinue_info("Client thread is exiting.");

    jinue_exit_thread();
}

static void run_ipc_test(void) {
    char recv_data[64];

    if(! bool_getenv("RUN_TEST_IPC")) {
        return;
    }

    jinue_info("Running threading and IPC test...");

    int fd = jinue_create_ipc(JINUE_IPC_NONE, &errno);

    if(fd < 0) {
        jinue_info("Creating IPC object descriptor.");
        jinue_error("Error number: %i", errno);
        return;
    }

    jinue_info("Main thread got descriptor %i.", fd);

    jinue_create_thread(ipc_test_client_thread, &ipc_test_thread_stack[THREAD_STACK_SIZE], NULL);

    jinue_buffer_t recv_buffer;
    recv_buffer.addr = recv_data;
    recv_buffer.size = sizeof(recv_data);

    jinue_message_t message;
    message.recv_buffers        = &recv_buffer;
    message.recv_buffers_length = 1;

    intptr_t ret = jinue_receive(fd, &message, &errno);

    if(ret < 0) {
        jinue_error("jinue_receive() failed with error: %i.", errno);
        return;
    }

    int function = message.recv_function;

    if(function != MSG_FUNC_TEST) {
        jinue_error("jinue_receive() unexpected function number: %i.", function);
        return;
    }

    jinue_info("Main thread received message:");
    jinue_info("  data:             \"%s\"", recv_data);
    jinue_info("  size:             %" PRIuPTR, ret);
    jinue_info("  function:         %u (user base + %u)", function, function - JINUE_SYS_USER_BASE);
    jinue_info("  cookie:           %" PRIuPTR, message.recv_cookie);
    jinue_info("  reply max. size:  %" PRIuPTR, message.reply_max_size);

    const char reply_string[] = "Hi, Main Thread!";

    jinue_const_buffer_t reply_buffer;
    reply_buffer.addr = reply_string;
    reply_buffer.size = sizeof(reply_string);   /* includes NUL terminator */

    jinue_message_t reply;
    reply.send_buffers          = &reply_buffer;
    reply.send_buffers_length   = 1;

    ret = jinue_reply(&reply, &errno);

    if(ret < 0) {
        jinue_error("jinue_reply() failed with error: %i.", errno);
    }

    jinue_info("Main thread is running.");
}

int main(int argc, char *argv[]) {
    char call_buffer[CALL_BUFFER_SIZE];
    int status;

    /* Say hello.
     *
     * We shouldn't do this before the call to jinue_set_syscall_implementation().
     * It works because the system call implementation selected before the call
     * is made is the interrupt-based one, which is slower but always available.
     *
     * TODO once things stabilize, ensure jinue_syscall() fails if no system
     * call implementation was set. */
    jinue_info("Jinue user space loader (%s) started.", argv[0]);

    dump_cmdline_arguments(argc, argv);
    dump_environ();
    dump_auxvec();

    /* Get system call implementation from auxiliary vectors so we can use
     * something faster than the interrupt-based one if available and ensure the
     * one we attempt to use is supported. */
    errno           = 0;
    int howsyscall  = getauxval(JINUE_AT_HOWSYSCALL);
    int ret         = jinue_set_syscall_implementation(howsyscall, &errno);

    if (ret < 0) {
        /* TODO map error numbers to name */
        jinue_error("Could not set system call implementation: %i", errno);
    }

    jinue_info("Using system call implementation '%s'.", syscall_implementation_name(howsyscall));

    /* get free memory blocks from microkernel */
    status = jinue_get_user_memory((jinue_mem_map_t *)&call_buffer, sizeof(call_buffer), &errno);

    if(status != 0) {
        jinue_error("error: could not get physical memory map from microkernel.");

        return EXIT_FAILURE;
    }

    dump_phys_memory_map((jinue_mem_map_t *)&call_buffer);

    run_ipc_test();

    if(bool_getenv("DEBUG_DO_REBOOT")) {
        jinue_info("Rebooting.");
        jinue_reboot();
    }

    while (1) {
        jinue_yield_thread();
    }
    
    return EXIT_SUCCESS;
}
