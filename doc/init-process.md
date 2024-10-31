# Initial Process Execution Environment

This document describes the execution environment set up for the initial
process by the user space loader.

## Program Binary

During the boot process, the user space loader loads a static ELF 
binary file from the initial RAM disk. The full path of the binary is 
specified by the `init` option on the kernel [command line](cmdline.md) 
and defaults to `/sbin/init` if that option isn't specified.

The user space sets up the program segments as described in the program 
headers of the ELF file. In addition, it sets up a stack for the 
initial thread.

## Initial Stack Layout

The initial stack is located at the very top of the user space virtual 
address space ([JINUE_KLIMIT](../include/jinue/shared/asm/i686.h)). 
Its size is 128KB 
([JINUE_STACK_SIZE](../include/jinue/shared/asm/i686.h), with 32KB 
([JINUE_RESERVED_STACK_SIZE](../include/jinue/shared/asm/i686.h)) of 
that size reserved for the command line arguments, environment 
variables and auxiliary vectors.

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
    +===========================================+ JINUE_KLIMIT - 32KB
    |                                           | initial stack pointer
    |                                           |
    |                                           |
    |             free stack space              |
    |                                           |
    |                                           |
    |                                           |
    |                                           |
    |                                           |
    +===========================================+ JINUE_KLIMIT - 128KB
```

## Command Line Arguments and Environment Variables

All command line arguments and environment variables come from the 
kernel command line. The [Kernel Command Line](cmdline.md) 
documentation describes which parameters are interpreted as command 
line arguments, which ones are environment variables and which ones are 
filtered out.

## Auxiliary Vectors

TODO

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

### Get Memory Information

Message number 4096 (JINUE_MSG_GET_MEMINFO) allows the initial process 
to request information from the loader regarding memory that has been 
used by the loader to extract the initial RAM disk and for the initial 
process itself. This information should be used alongside the 
information from the [GET_USER_MEMORY](syscalls/get-user-memory.md) 
system call to determine what memory is available for use.

**TODO describe the reply structure**

### Exit Loader

Message number 4097 (JINUE_MSG_EXIT) requests that the loader exits.

Upon receiving this message, the loader exits *without* sending a 
reply, which means the caller should expect the 
[SEND](syscalls/send.md) system call to fail with error number 
`JINUE_EIO`. This is done on purpose to ensure the calling thread 
remains blocked until the loader has actually exited. If the loader 
were to send a reply and then exit, there would exist a race condition 
where the initial process could start reusing the memory reclaimed from 
the loader after the loader has sent the reply but before it actually 
exited.

## Initialization

**TODO provide and example of the initialization sequence**
