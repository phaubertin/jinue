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
#include <sys/mman.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "utils.h"

#define MAP_BUFFER_SIZE 16384

#define PRETTY_MODE_SIZE 11

extern char **environ;

extern const Elf32_auxv_t *_jinue_libc_auxv;

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
            {"AT_PHDR",         JINUE_AT_PHDR},
            {"AT_PHENT",        JINUE_AT_PHENT},
            {"AT_PHNUM",        JINUE_AT_PHNUM},
            {"AT_PAGESZ",       JINUE_AT_PAGESZ},
            {"AT_ENTRY",        JINUE_AT_ENTRY},
            {"AT_STACKBASE",    JINUE_AT_STACKBASE},
            {"AT_HOWSYSCALL",   JINUE_AT_HOWSYSCALL},
            {"AT_ACPI_RSDP",    JINUE_AT_ACPI_RSDP},
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
            [JINUE_I686_HOWSYSCALL_INTERRUPT]      = "interrupt",
            [JINUE_I686_HOWSYSCALL_FAST_AMD]       = "SYSCALL/SYSRET (fast AMD)",
            [JINUE_I686_HOWSYSCALL_FAST_INTEL]     = "SYSENTER/SYSEXIT (fast Intel)"
    };

    if(implementation < 0 || implementation > JINUE_I686_HOWSYSCALL_LAST) {
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

static const char *phys_memory_type_description(uint32_t type) {
    switch(type) {
    case JINUE_MEMYPE_MEMORY:
        return "Available";
    case JINUE_MEMYPE_RESERVED:
        return "Reserved";
    case JINUE_MEMYPE_ACPI:
        return "Reserved/ACPI";
    case JINUE_MEMYPE_NVS:
        return "Reserved/NVS";
    case JINUE_MEMYPE_UNUSABLE:
        return "Reserved/Unusable";
    case JINUE_MEMYPE_DISABLED:
        return "Reserved/Disabled";
    case JINUE_MEMYPE_PERSISTENT:
        return "Available/Persistent";
    case JINUE_MEMYPE_OEM:
        return "Reserved/OEM";
    case JINUE_MEMYPE_KERNEL_RESERVED:
        return "Jinue/Reserved";
    case JINUE_MEMYPE_KERNEL_IMAGE:
        return "Jinue/Kernel Image";
    case JINUE_MEMYPE_RAMDISK:
        return "Jinue/Compressed RAM Disk";
    case JINUE_MEMYPE_LOADER_AVAILABLE:
        return "Jinue/Available (Loader Hint)";
    default:
        /* escaped to prevent trigraph */
        return "Reserved (??\?)";
    }
}

static void dump_phys_memory_map(const jinue_addr_map_t *map) {
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
        const jinue_addr_map_entry_t *entry = &map->entry[idx];

        if(entry->type == JINUE_MEMYPE_MEMORY || !ram_only) {
            jinue_info(
                    "  %c [%016" PRIx64 "-%016" PRIx64 "] %s",
                    (entry->type==JINUE_MEMYPE_MEMORY) ? '*' : ' ',
                    entry->addr,
                    entry->addr + entry->size - 1,
                    phys_memory_type_description(entry->type)
            );
        }
    }
}

void dump_address_map(void) {
    char map_buffer[MAP_BUFFER_SIZE];

    jinue_buffer_t call_buffer;
    call_buffer.addr = map_buffer;
    call_buffer.size = sizeof(map_buffer);

    /* get free memory blocks from microkernel */
    int status = jinue_get_address_map(&call_buffer, &errno);

    if(status != 0) {
        jinue_error("error: could not get memory map from kernel: %s", strerror(errno));
        return;
    }

    const jinue_addr_map_t *map = (const jinue_addr_map_t *)&map_buffer;
    dump_phys_memory_map(map);
}

static const char *segment_type_description(int type) {
    switch(type) {
        case JINUE_SEG_TYPE_RAMDISK:
            return "extracted RAM disk";
        case JINUE_SEG_TYPE_FILE:
            return "file";
        case JINUE_SEG_TYPE_ANON:
            return "anonymous";
        default:
            return "???";
    }
}

static const char *pretty_permissions(int prot) {
    const int mask = PROT_READ | PROT_WRITE | PROT_EXEC;

    static char buffer[5];
    buffer[0] = (prot & PROT_READ)  ? 'r' : '-';
    buffer[1] = (prot & PROT_WRITE) ? 'w' : '-';
    buffer[2] = (prot & PROT_EXEC)  ? 'x' : '-';
    buffer[3] = ((prot & ~mask) != 0) ? '?' : '\0';
    buffer[4] = '\0';

    return buffer;
}

static void dump_segment(const jinue_meminfo_t *meminfo, unsigned int index) {
    const jinue_segment_t *segment = jinue_get_segment(meminfo, index);

    jinue_info("    [%3d] Physical address: %#" PRIx64, index, segment->addr);
    jinue_info("          Size:             %#" PRIx64 " %" PRIu64, segment->size, segment->size);
    jinue_info("          Type:             %s", segment_type_description(segment->type));
}

static void dump_mapping(const jinue_meminfo_t *meminfo, unsigned int index) {
    const jinue_mapping_t *mapping = jinue_get_mapping(meminfo, index);

    jinue_info("    [%3d] Virtual address:  %#p", index, mapping->addr);
    jinue_info("          Size:             %#zx %zu", mapping->size, mapping->size);
    jinue_info("          Segment index:    %d", mapping->segment);
    jinue_info("          Offset:           %#zx %zu", mapping->offset, mapping->offset);
    jinue_info("          Permissions:      %s", pretty_permissions(mapping->perms));

}

static void dump_meminfo(const jinue_meminfo_t *meminfo) {
    
    if(! bool_getenv("DEBUG_DUMP_LOADER_MEMORY_INFO")) {
        return;
    }
    
    jinue_info("Memory and mappings information from user space loader:");
    jinue_info("  Extracted RAM disk:");
    dump_segment(meminfo, meminfo->ramdisk);

    jinue_info("  Hints:");
    jinue_info("    Physical allocation start: %#" PRIx64, meminfo->hints.physaddr);
    jinue_info("    Physical allocation limit: %#" PRIx64, meminfo->hints.physlimit);

    jinue_info("  Segments:");

    for(unsigned int idx = 0; idx < meminfo->n_segments; ++idx) {
        dump_segment(meminfo, idx);
    }

    jinue_info("  Mappings:");

    for(unsigned int idx = 0; idx < meminfo->n_mappings; ++idx) {
        dump_mapping(meminfo, idx);
    }
}

void dump_loader_memory_info(void) {
    char buffer[MAP_BUFFER_SIZE];

    const jinue_meminfo_t *meminfo = jinue_get_meminfo(buffer, sizeof(buffer));

    if(meminfo == NULL) {
        return;
    }

    dump_meminfo(meminfo);
}

static const char *pretty_mode(char *mode, const jinue_dirent_t *dirent) {
    switch(dirent->type) {
    case JINUE_DIRENT_TYPE_FILE:
        mode[0] = '-';
        break;
    case JINUE_DIRENT_TYPE_DIR:
        mode[0] = 'd';
        break;
    case JINUE_DIRENT_TYPE_SYMLINK:
        mode[0] = '1';
        break;
    case JINUE_DIRENT_TYPE_CHARDEV:
        mode[0] = 'c';
        break;
    case JINUE_DIRENT_TYPE_BLKDEV:
        mode[0] = 'b';
        break;
    case JINUE_DIRENT_TYPE_FIFO:
        mode[0] = 'p';
        break;
    default:
        mode[0] = '?';
        break;
    }

    struct {int pos; int c; int mask;} map[] = {
            {1, 'r', JINUE_IRUSR},
            {2, 'w', JINUE_IWUSR},
            {3, 's', JINUE_IXUSR | JINUE_ISUID},
            {3, 'S', JINUE_ISUID},
            {3, 'x', JINUE_IXUSR},
            {4, 'r', JINUE_IRGRP},
            {5, 'w', JINUE_IWGRP},
            {6, 's', JINUE_IXGRP | JINUE_ISGID},
            {6, 'S', JINUE_ISGID},
            {6, 'x', JINUE_IXGRP},
            {7, 'r', JINUE_IROTH},
            {8, 'w', JINUE_IWOTH},
            {9, 'x', JINUE_IXOTH}
    };

    for(int idx = 0; idx < sizeof(map) / sizeof(map[0]); ++idx) {
        if((dirent->mode & map[idx].mask) == map[idx].mask) {
            mode[map[idx].pos] = map[idx].c;
        } else {
            mode[map[idx].pos] = '-';
        }
    }

    mode[10] = '\0';

    return mode;
}

static void dump_ramdisk(const jinue_dirent_t *root) {
    char mode_buffer[PRETTY_MODE_SIZE];

    if(! bool_getenv("DEBUG_DUMP_RAMDISK")) {
        return;
    }

    jinue_info("RAM disk dump:");

    const jinue_dirent_t *dirent = jinue_dirent_get_first(root);

    while(dirent != NULL) {
        jinue_info(
                "  %s %6" PRIu32 " %6" PRIu32 " %12" PRIu64 " %s",
                pretty_mode(mode_buffer, dirent),
                dirent->uid,
                dirent->gid,
                dirent->size,
                jinue_dirent_name(dirent)
        );

        dirent = jinue_dirent_get_next(dirent);
    }
}

void dump_loader_ramdisk(void) {
    static const jinue_dirent_t *root = MAP_FAILED;

    if(root == MAP_FAILED) {
        char buffer[MAP_BUFFER_SIZE];

        const jinue_meminfo_t *meminfo = jinue_get_meminfo(buffer, sizeof(buffer));

        if(meminfo == NULL) {
            return;
        }

        const jinue_segment_t *ramdisk = jinue_get_ramdisk(meminfo);

        root = mmap(NULL, ramdisk->size, PROT_READ, MAP_SHARED, -1, ramdisk->addr);
    }

    if(root == MAP_FAILED) {
        jinue_error("error: mmap() failed: %s.", strerror(errno));
        return;
    }

    dump_ramdisk(root);
}
