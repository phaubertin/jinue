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
wherever it wants, but if the initial process makes use of the Jinue
libc, they will be located as shown below.

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
the stack is reserved by the user space loader for the command line
arguments, the environment variables and auxiliary vectors. On program
entry, the stack pointer is set to the limit of this reserved region.

More specifically, the initial stack layout and contents is illustrated 
below. Starting from the top of the stack on program entry (so from the 
lowest address), the stack contents is, in this order:

* A C language `int` value that indicates the number of command line
  arguments (argc), including the program name (`argv[0]]`).
* A string pointers table with `argc + 1` elements that points to the
  command line arguments. The last element of this table contains the
  value 0 (i.e. the C language `NULL` pointer value).
* A C language `int` value that indicates the number of environment
  variables.
* A string pointers table that points to the environment variables. Just
  like the command line arguments table, there is one element than the
  number of environment variables, with the last element set to zero.
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

The value of the `JINUE_AT_HOWSYSCALL` auxiliary vector identifies the
system call implementation to use on architecture where there can be
more than one. Its meaning is implementation dependent. See
[System Call Implementations](syscalls/implementations.md) for detail.

## Initial Descriptors

The user space loader sets up the following initial descriptors.

| Descriptor<br>Number | Name                         | Description         |
|----------------------|------------------------------|---------------------|
| 3                    | `JINUE_DESC_SELF_PROCESS`    | Self process        |
| 4                    | `JINUE_DESC_LOADER_ENDPOINT` | Loader IPC endpoint |

### Self Process Descriptor

The self process descriptor is a descriptor that references the initial
process' own process. It can be used, among other things, to [map
memory](syscalls/mmap.md) into the process' address space and to
[create new threads](syscalls/create-thread.md).

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

### Get Memory Information (`JINUE_MSG_GET_MEMINFO`)

Message number 4096 (JINUE_MSG_GET_MEMINFO) allows the initial process
to request information from the loader regarding memory it has used to
extract the initial RAM disk and for the initial process itself. This
information should be used alongside the information from the
[GET_USER_MEMORY](syscalls/get-user-memory.md) system call to determine
what memory is available for use.

The reply contains the following, concatenated, in this order:
* A memory information structure (`jinue_loader_meminfo_t`).
* A variable number of segment structures (`jinue_loader_segment_t`).
* A variable number of mapping structures (`jinue_loader_vmap_t`).

The number of segment and mapping structures is indicated in the memory information structure.

```
    +===============================+
    | Memory information structure: |
    | * n_segments: N               |
    | * n_vmaps: M                  |
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

**TODO describe the reply structure**

Note the loader does not report information about memory it itself is
using. The initial process is expected to get the information it needs
with this message and then request that the loader exit to reclaim that
memory. Before the loader has exited, it is safe to start allocating 
memory only starting from the physical allocation address hint provided
in the memory information structure.

### Exit Loader (`JINUE_MSG_EXIT`)

Message number 4097 (JINUE_MSG_EXIT) requests that the loader exit.

Upon receiving this message, the loader exits *without* sending a
reply, which means the caller should expect the
[SEND](syscalls/send.md) system call to fail with error number
`JINUE_EIO`. This is done on purpose to ensure the calling thread
remains blocked until the loader has actually exited. If the loader
were to send a reply and then exit, there would exist a race condition
where the initial process could start reusing the memory reclaimed from
the loader after the loader has sent the reply but before it actually
exited.

