/*
 * Copyright (C) 2023 Philippe Aubertin.
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

#include <jinue/jinue.h>
#include <jinue/loader.h>
#include <jinue/utils.h>
#include <sys/auxv.h>
#include <sys/elf.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "utils.h"

extern char **environ;

extern const Elf32_auxv_t *_jinue_libc_auxv;

static const char *jinue_phys_mem_type_description(uint32_t type) {
    switch(type) {
    case JINUE_MEM_TYPE_AVAILABLE:
        return "Available";
    case JINUE_MEM_TYPE_BIOS_RESERVED:
        return "Unavailable/Reserved";
    case JINUE_MEM_TYPE_ACPI:
        return "Unavailable/ACPI";
    case JINUE_MEM_TYPE_RAMDISK:
        return "Compressed RAM Disk";
    case JINUE_MEM_TYPE_KERNEL_IMAGE:
        return "Kernel Image";
    case JINUE_MEM_TYPE_KERNEL_RESERVED:
        return "Unavailable/Kernel Data";
    case JINUE_MEM_TYPE_LOADER_AVAILABLE:
        return "Available (Loader Hint)";
    default:
        return "Unavailable/???";
    }
}

void dump_phys_memory_map(const jinue_mem_map_t *map) {
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

        if(entry->type == JINUE_MEM_TYPE_AVAILABLE || !ram_only) {
            jinue_info(
                    "  %c [%016" PRIx64 "-%016" PRIx64 "] %s",
                    (entry->type==JINUE_MEM_TYPE_AVAILABLE)?'*':' ',
                    entry->addr,
                    entry->addr + entry->size - 1,
                    jinue_phys_mem_type_description(entry->type)
            );
        }
    }
}

void dump_cmdline_arguments(int argc, char *argv[]) {
    if(! bool_getenv("DEBUG_DUMP_CMDLINE_ARGS")) {
        return;
    }

    jinue_info("Command line arguments:");

    for(int idx = 0; idx < argc; ++idx) {
        jinue_info("  %s", argv[idx]);
    }
}

void dump_environ(void) {
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
            {"AT_NULL",         JINUE_AT_NULL},
            {"AT_IGNORE",       JINUE_AT_IGNORE},
            {"AT_EXECFD",       JINUE_AT_EXECFD},
            {"AT_PHDR",         JINUE_AT_PHDR},
            {"AT_PHENT",        JINUE_AT_PHENT},
            {"AT_PHNUM",        JINUE_AT_PHNUM},
            {"AT_PAGESZ",       JINUE_AT_PAGESZ},
            {"AT_BASE",         JINUE_AT_BASE},
            {"AT_FLAGS",        JINUE_AT_FLAGS},
            {"AT_ENTRY",        JINUE_AT_ENTRY},
            {"AT_STACKBASE",    JINUE_AT_STACKBASE},
            {"AT_HOWSYSCALL",   JINUE_AT_HOWSYSCALL},
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
            [JINUE_SYSCALL_IMPL_INTERRUPT]      = "interrupt",
            [JINUE_SYSCALL_IMPL_FAST_AMD]       = "SYSCALL/SYSRET (fast AMD)",
            [JINUE_SYSCALL_IMPL_FAST_INTEL]     = "SYSENTER/SYSEXIT (fast Intel)"
    };

    if(implementation < 0 || implementation > JINUE_SYSCALL_IMPL_LAST) {
        return "?";
    }

    return names[implementation];
}

void dump_auxvec(void) {
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

void dump_syscall_implementation(void) {
    if(! bool_getenv("DEBUG_DUMP_SYSCALL_IMPLEMENTATION")) {
        return;
    }

    int howsyscall = getauxval(JINUE_AT_HOWSYSCALL);
    jinue_info("Using system call implementation '%s'.", syscall_implementation_name(howsyscall));
}
