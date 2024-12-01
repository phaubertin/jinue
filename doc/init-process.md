# Initial Process Execution Environment

This document describes the execution environment that the user space
loader sets up for the initial process during the boot sequence.

When implementing a chain loader, for example one that reads the initial
program binary from disk instead of the initial RAM disk image, we
suggest that the chain loader set up the same execution environment.

## Program Binary

The user space loader loads a static ELF binary file from the initial
RAM disk image. The full path of the binary is specified by the `init`
option on the kernel [command line](cmdline.md) and defaults to
`/sbin/init` if that option isn't specified.

## Address Space Layout

The user space loader sets up the program segments as described in the
program headers of the program binary and sets up a stack for the
initial program thread.

The initial stack is located at the very top of the user address space
([JINUE_KLIMIT](../include/jinue/shared/asm/i686.h)) and its size is
128kB ([JINUE_STACK_SIZE](../include/jinue/shared/asm/stack.h)).

A typical address space for the initial process is illustrated below.
Only the kernel reserved region and the location of the stack are
imposed by the kernel and the user space loader, respectively. The
location of the program segments are determined entirely by the program
binary. The initial process is free to place its heap and mmap() area
wherever it wants, but if the initial process makes use of the
[Jinue libc](../userspace/lib/libc), they will be located as shown
below.

```
    +===========================================+
    |///////////////////////////////////////////|
    |///////                             ///////|
    |///////   Reserved for kernel use   ///////|
    |///////                             ///////|
    |///////////////////////////////////////////|
    +===========================================+ JINUE_KLIMIT
    |                                           |
    |                   stack                   |
    |                                           |
    +vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv+ JINUE_KLIMIT - 128kB
    |///////////////////////////////////////////|
    |///////////////////////////////////////////|
    |///////////////////////////////////////////|
    |///////////////////////////////////////////|
    |///////////////////////////////////////////|
    |^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^|
    |                                           |
    |               mmap() area                 |
    |                                           |
    +-------------------------------------------+ MMAP_BASE
    |///////////////////////////////////////////|
    |///////////////////////////////////////////|
    |///////////////////////////////////////////|
    |///////////////////////////////////////////|
    |///////////////////////////////////////////|
    |^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^|
    |                                           |
    |                  heap                     |
    |                                           |
    +-------------------------------------------+ program break
    |        program data (data segment)        |
    +-------------------------------------------+
    |        program code (text segment)        |
    +-------------------------------------------+
    |///////////////////////////////////////////|
    |///////////////////////////////////////////|
    |///////////////////////////////////////////|
    +===========================================+ 0
```


## Initial Stack Layout

The higher 32kB
([JINUE_RESERVED_STACK_SIZE](../include/jinue/shared/asm/stack.h)) of
the stack is reserved and used by the user space loader for the command
line arguments, the environment variables and auxiliary vectors. On
program entry, the stack pointer is set to the limit of this reserved
region.

More specifically, the initial stack layout and contents is illustrated
below. Starting from the top of the stack on program entry (so from the
lowest address), the stack contents is, in this order:

* A C language `int` value that indicates the number of command line
  arguments (`argc`), including the program name (`argv[0]]`).
* A string pointers table with `argc + 1` elements that points to the
  command line arguments (`argv`). The last element of this table
  contains the value 0 (i.e. the C language `NULL` pointer value).
* A C language `int` value that indicates the number of environment
  variables.
* A string pointers table that points to the environment variables. Just
  like the command line arguments table, there is one more element than
  the number of environment variables, with the last element set to
  zero.
* The auxiliary vectors (described below).
* The command line argument strings, indexed by the command line
  argument strings table described above.
* The environment variable strings, indexed by the environment variable
  strings table described above.

```
    +===========================================+ JINUE_KLIMIT
    |                                           |
    |                                           |
    |           unused reserved space           |
    |                                           |
    |                                           |
    +-------------------------------------------+
    |      environment variables strings        |
    |                                           |
    +-------------------------------------------+
    |     command line arguments strings        |
    |                                           |
    +-------------------------------------------+
    |             auxiliary vectors             |
    |                                           |
    +-------------------------------------------+
    |       environment variables table         |
    |                                           |
    +-------------------------------------------+
    |     number of environment variables       |
    +-------------------------------------------+
    |   command line arguments table (argv)     |
    |                                           |
    +-------------------------------------------+
    |  number of command line arguments (argc)  |
    +===========================================+ JINUE_KLIMIT - 32kB
    |                                           | initial stack pointer
    |                                           |
    |                                           |
    |                                           |
    |                                           |
    |             free stack space              |
    |                                           |
    |                                           |
    |                                           |
    |                                           |
    |                                           |
    +===========================================+ JINUE_KLIMIT - 128kB
```

All command line arguments and environment variables come from parsing
the kernel command line. The [Kernel Command Line](cmdline.md)
documentation describes which parameters are interpreted as command
line arguments, which ones are environment variables and which ones are
filtered out.

## Auxiliary Vectors

The auxiliary vectors are a table of type-value pairs. Each type is represented
by a C language `ìnt` value and each value is a 32-bit integer.

The following table lists the auxiliary vectors:

| Type Value | Type Name             | Description                               |
|------------|-----------------------|-------------------------------------------|
| 0          | `JINUE_AT_NULL`       | Indicates the last vector, discard value  |
| 1          | `JINUE_AT_IGNORE`     | Ignore                                    |
| 2          | `JINUE_AT_PHDR`       | Address of ELF program headers            |
| 3          | `JINUE_AT_PHENT`      | Size of a program header entry            |
| 4          | `JINUE_AT_PHNUM`      | Number of program headers                 |
| 5          | `JINUE_AT_PAGESZ`     | Page size                                 |
| 6          | `JINUE_AT_ENTRY`      | Address of program entry point            |
| 7          | `JINUE_AT_STACKBASE`  | Stack base address                        |
| 8          | `JINUE_AT_HOWSYSCALL` | System call implementation                |
| 9          | `JINUE_AT_ACPI_RSDP`  | Physical address of ACPI RSDP             |

The value of the `JINUE_AT_HOWSYSCALL` auxiliary vector identifies the
system call implementation to use on architectures where there can be
more than one. Its meaning is architecture dependent. See
[System Call Implementations](syscalls/implementations.md) for detail.

## Initial Descriptors

The user space loader sets up the following initial descriptors.

| Descriptor<br>Number | Name                         | Description         |
|----------------------|------------------------------|---------------------|
| 0                    | `JINUE_DESC_SELF_PROCESS`    | Self process        |
| 1                    | `JINUE_DESC_MAIN_THREAD`     | Main thread         |
| 2                    | `JINUE_DESC_LOADER_ENDPOINT` | Loader IPC endpoint |

### Self Process Descriptor

The self process descriptor is a descriptor that references the initial
process' own process. It can be used, among other things, to [map
memory](syscalls/mmap.md) into the process' address space and to
[create new threads](syscalls/create-thread.md).

### Main Thread Descriptor

The main thread descriptor is a descriptor that references the initial
thread in the process. It can be used, among other things, to
[await](syscalls/await-thread.md) the initial thread from another thread.

### Loader IPC Endpoint Descriptor

This descriptor refers to an Inter-Process Communications (IPC)
endpoint and allows the initial process to [send
messages](syscalls/send.md) to the loader. The protocol for this
endpoint is described below.

## Loader IPC Protocol

Once the loader is done loading the initial process, it remains
resident and waits for messages on an Inter-Process Communications
(IPC) endpoint. This section describes the protocol for this endpoint.

In the following description, a "message number" refers to the function
number above 4096 specified when invoking the [SEND](syscalls/send.md)
system call and passed along to the receiving thread.

### Overview

The initial process should request information about memory usage
using the `JINUE_MSG_GET_MEMINFO` message. It should then ask the loader
to exit using the `JINUE_MSG_EXIT` message.

### Get Memory Information (`JINUE_MSG_GET_MEMINFO`)

An empty message with message number 4096 (JINUE_MSG_GET_MEMINFO)
allows the initial process to request information from the loader
regarding memory it has used to extract the initial RAM disk and for
the initial process itself. This information should be used alongside
the information from the [GET_ADDRESS_MAP](syscalls/get-address-map.md)
system call to determine what memory is available for use.

Note the loader does not report information about memory it itself is
using. The initial process is expected to get the information it needs
with this message and then request that the loader exit to reclaim that
memory. Before the loader has exited, it is safe to start allocating
memory only starting from the physical allocation address hint provided
in the memory information structure. Otherwise, the initial process
risks overwriting memory the loader is still actively using.

The reply to this message contains the following, concatenated, in this
order:

* A memory information structure (`jinue_meminfo_t`).
* A variable number of segment structures (`jinue_segment_t`).
* A variable number of mapping structures (`jinue_mapping_t`).

The number of segment and mapping structures is indicated in the memory
information structure. For the definitions of all these structures, see
[<jinue/loader.h>](../include/jinue/loader.h).

```
    +===============================+
    | Memory information structure: |
    | * n_segments: N               |
    | * n_mappings: M               |
    | * ...                         |
    |                               |
    |                               |
    +===============================+
    | Segment 0                     |
    +-------------------------------+
    | Segment 1                     |
    +-------------------------------+
    |  ...                          |
    +-------------------------------+
    | Segment N-1                   |
    +===============================+
    | Mapping 0                     |
    +-------------------------------+
    | Mapping 1                     |
    +-------------------------------+
    |  ...                          |
    +-------------------------------+
    | Mapping M-1                   |
    +===============================+
```

#### The Memory Information Structure

The memory information structure contains the following members:

* `n_segments` number of segment structures.
* `n_mappings` number of mapping structures.
* `ramdisk`index of the segment structure that represent the extracted
  RAM disk.
* `hints` contains advice the initial process is free to use or
  disregard:
  * `physaddr` physical address from which to start allocating memory
    sequentially.
  * `physlimit`physical address beyond which the initial process should
    not allocate memory sequentially starting from `physaddr`.

As long as the loader has exited, the initial process is free to 
allocate memory anywhere that is described as user memory by the kernel 
memory map (see the [GET_ADDRESS_MAP](syscalls/get-address-map.md) 
system call) and that is not identified as in use by one of the segment 
structures. It is also free to either map the extracted RAM disk for 
its own use or instead reclaim that memory. However, early in its 
startup process, the initial process might need to allocate memory 
before it has processed all this information and while the loader is 
still resident. It is in this situation that the `physaddr` and 
`physlimit` hints are most useful.

#### The Segment Structure

Each segment structure describes a range of memory that is used either
by the initial process or by the extracted RAM disk. Its members are as
follow:

* `addr` physical address of the start of the segment
* `size` size of the segment in bytes
* `type`type of the segment

Segment types:

| Number | Name                     | Description            |
|--------|--------------------------|------------------------|
| 0      | `JINUE_SEG_TYPE_RAMDISK` | The extracted RAM disk |
| 1      | `JINUE_SEG_TYPE_FILE`    | A file                 |
| 2      | `JINUE_SEG_TYPE_ANON`    | Anonymous memory       |

Segments may overlap in the following ways:

* File segments may overlap the extracted RAM disk segment (if they are
  files in the extracted RAM disk).
* Future direction: if the RAM disk image has the right format and is
  uncompressed, the memory range of the extracted RAM disk might be
  identical to the RAM disk image as described by the
  [GET_ADDRESS_MAP](syscalls/get-address-map.md) system call, i.e. the
  loader might be using the RAM disk image in place.

#### The Mapping Structure

Each mapping structure describes a mapping in the initial process'
address space. Its members are as follow:

* `addr` virtual address of the mapping
* `size` size of the mapping in bytes
* `segment` index of the segment this mapping references
* `offset` offset of the mapping from the start of the referenced
  segment
* `perms` permissions of the mapping: logical OR of `JINUE_PROT_READ`,
  `JINUE_PROT_WRITE` and/or `JINUE_PROT_EXEC`

### Exit Loader (`JINUE_MSG_EXIT`)

An empty message with message number 4097 (JINUE_MSG_EXIT) requests that
the loader exit.

Upon receiving this message, the loader exits *without* sending a
reply, which means the caller should expect the
[SEND](syscalls/send.md) system call to fail with error number
`JINUE_EIO`. This is done on purpose to ensure the calling thread
remains blocked until the loader has actually exited. If the loader
were to send a reply and then exit, there would be a race condition
where the initial process could start reusing the memory reclaimed from
the loader after the loader has sent the reply but before it actually
exited.

### Future Direction

Messages will be added to this endpoint that will allow file and
anonymous mappings to be added through the loader. This will be done to
provide support for a dynamic loader so the initial process can get a
full accounting of what has been mapped into its address space,
including by the dynamic loader, through the `JINUE_MSG_GET_MEMINFO`
message. This would also allow an application or a dynamic loader to
take advantage of the abilities of a chain loader that follows this
protocol, for example a chain loader's abilities to read files from a
disk or a network.
