# System Call Interface

## Quick Reference

### System Call Reference

| Number  | Name                                    | Description                          |
|---------|-----------------------------------------|--------------------------------------|
| 0       | -                                       | Reserved                             |
| 1       | -                                       | Reserved                             |
| 2       | [REBOOT](reboot.md)                     | Reboot the System                    |
| 3       | [PUTS](puts.md)                         | Write string to console              |
| 4       | [CREATE_THREAD](create-thread.md)       | Create a thread                      |
| 5       | [YIELD_THREAD](yield-thread.md)         | Yield the current thread             |
| 6       | [SET_THREAD_LOCAL](set-thread-local.md) | Set Thread-Local Storage (TLS)       |
| 7       | -                                       | Reserved                             |
| 8       | [GET_ADDRESS_MAP](get-address-map.md)   | Get memory address map               |
| 9       | [CREATE_ENDPOINT](create-endpoint.md)   | Create IPC endpoint                  |
| 10      | [RECEIVE](receive.md)                   | Receive message                      |
| 11      | [REPLY](reply.md)                       | Reply to message                     |
| 12      | [EXIT_THREAD](exit-thread.md)           | Terminate the current thread         |
| 13      | [MMAP](mmap.md)                         | Map memory                           |
| 14      | [CREATE_PROCESS](create-process.md)     | Create a process                     |
| 15      | [MCLONE](mclone.md)                     | Clone a memory mapping               |
| 16      | [DUP](dup.md)                           | Duplicate a descriptor               |
| 17      | [CLOSE](close.md)                       | Close a descriptor                   |
| 18      | [DESTROY](destroy.md)                   | Destroy a kernel object              |
| 19      | [MINT](mint.md)                         | Mint a descriptor                    |
| 20      | [START_THREAD](start-thread.md)         | Start a thread                       |
| 21      | [AWAIT_THREAD](await-thread.md)         | Wait for a thread to exit            |
| 22      | [REPLY_ERROR](reply-error.md)           | Reply to message with an error       |
| 23      | [SET_ACPI](set-acpi.md)                 | Provide mapped ACPI tables           |
| 24-4095 | -                                       | Reserved                             |
| 4096+   | [SEND](send.md)                         | Send a message                       |

#### Reserved Function Numbers

Any function marked as reserved returns -1 (in `arg0`) and sets error number
`JINUE_ENOSYS` (in `arg1`).

### Error Reference

| Number | Name             | Description                            |
|--------|------------------|----------------------------------------|
| 1      |`JINUE_ENOMEM`    | Not enough memory                      |
| 2      |`JINUE_ENOSYS`    | Function not supported                 |
| 3      |`JINUE_EINVAL`    | Invalid argument                       |
| 4      |`JINUE_EAGAIN`    | Currently unavailable, try again later |
| 5      |`JINUE_EBADF`     | Bad/invalid descriptor                 |
| 6      |`JINUE_EIO`       | Input/output error                     |
| 7      |`JINUE_EPERM`     | Operation not permitted                |
| 8      |`JINUE_E2BIG`     | Not enough space in output buffer      |
| 9      |`JINUE_ENOMSG`    | No message                             |
| 10     |`JINUE_ENOTSUP`   | Not supported                          |
| 11     |`JINUE_EBUSY`     | Device or resource busy                |
| 12     |`JINUE_ESRCH`     | No such thread/process                 |
| 13     |`JINUE_EDEADLK`   | Resource deadlock would occur          |
| 14     |`JINUE_EPROTO`    | Protocol error                         |

## Overview

### System Call Registers

During a system call, information is passed back and forth between user space
and the microkernel through four pointer-sized logical registers named `arg0`
to `arg3`.

The mapping of these four system call registers to actual CPU registers is
architecture dependent. However, the system call specification given in terms
arguments and return values set in the system call registers is independent of
the architecture.

### System Call Implementations

The architecture-dependent implementations for invoking a system call and the
mapping of system call registers to actual architectural registers is described
in [System Call Implementations](implementations.md).

### Function Number and Arguments

When invoking a system call, `arg0` must be set to a function number that
identifies the specific function being called. Arguments for the call are passed
in `arg1` to `arg3`.

### Return Value

On return from a system call, the contents of `arg0` to `arg3` is set according
to the result of the system call. The interpretation of the values set in these
registers depends on the function number.

Most *but not all* system calls follow the following convention:

* `arg0` contains a return value, which should be cast to a pointer-sized signed
integer (C type `intptr_t`). If the value is positive (including zero), then the
call was successful. A non-zero negative value indicates an error has occurred.
* If the call failed, as indicated by a negative value in `arg0`, `arg1`
contains an [error number](#error-reference). Otherwise, `arg1` is set to zero.
* `arg2` and `arg3` are reserved and should be ignored.

### Inter-Process Communication (IPC)

The Jinue microkernel implements a synchronous message-passing mechanism for
inter-process communication. The client thread uses the [SEND](send.md) system
call to send a message to an IPC endpoint, which serves as a rendez-vous point
for IPC. The server thread uses the [RECEIVE](receive.md) system call to receive
a message from the IPC  endpoint, and then the [REPLY](reply.md) call to send
the reply or the [REPLY_ERROR](reply-error.md) call to send an error code.

System call function numbers 0 to 4095 inclusive are reserved by the microkernel
for the functions it implements. Function numbers 4096 and up all invoke the
[SEND](send.md) system call. The function number, in that context called the
message number, is passed as part of the message, which allows a server thread
to distinguish between multiple message types.

### Descriptors

The system call interface uses descriptors to refer to kernel resources, such as
threads, processes and IPC endpoints. A descriptor, similar to a Unix
[file descriptor](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap03.html#tag_03_166),
is a per-process unique, non-negative integer. It can be specified as an
argument to system calls.
