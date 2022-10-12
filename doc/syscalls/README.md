# System Call Interface

## Argument Registers

During a system call, information is passed back and forth between user space
and the microkernel through the four pointer-sized logical registers `arg0` to
`arg3`.

When invoking a system call, `arg0` contains a function number that identifies
the specific function being called. `arg1` to `arg3` contain arguments for the
call. 

Function numbers 0 to 4095 inclusive are reserved by the microkernel for the
functions it implements. Function numbers 4096 and up identify the Send Message
system call. The function number is passed to the recipient as part of the
message.

On return from a system call, the contents of arg0 to arg3 depends on the
function number. Most *but not all* system calls follow the following
convention:

* `arg0` contains a return value, which should be cast as a signed integer (C
type `int`). If the value is positive (including zero), then the call was
successful. A non-zero negative value indicates an error has occurred.
* If the call failed, as indicated by the value in `arg0`, `arg1` contains the
error number. Otherwise, it is set to zero.
* `arg2` and `arg3` are reserved and should be ignored.

## Descriptors

The system call interface uses descriptors to refer to kernel resources such as
threads, processes and Inter-Process Communication (IPC) endpoints. Descriptors,
similar to Unix
[file descriptors](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap03.html#tag_03_166),
are per-process unique, non-negative integer. They can be specified as arguments
to or returned by system calls. (TODO not yet implemented: They can also be
passed to another process through an IPC.)

## System Call Mechanisms

The microkernel supports the following mechanisms to invoke system calls. Not
all mechanisms are available on all CPUs. On initialization, before invoking any
other system call, an application should call the "Get System Call Mechanism"
system call to determine the best supported system call mechanism. This call of
the "Get System Call Mechanism" system call should be done using the
interrupt-based mechanism, which is the only one garanteed to be supported by
all CPUs.

### Interrupt-Based Mechanism

A system call can be invoked by generating a software interrupt to interrupt
vector 128 (80h). The argument registers are mapped as follow:

* `arg0` maps to CPU register `eax`.
* `arg1` maps to CPU register `ebx`.
* `arg2` maps to CPU register `esi`.
* `arg3` maps to CPU register `edi`.

This system call is always available. However, it is also the slowest, so it is
best to use another one if support is available.

### SYSCALL/SYSRET (Fast AMD) Mechanism

A system call can be invoked by executing the `SYSCALL` CPU instruction.

* `arg0` maps to CPU register `eax`.
* `arg1` maps to CPU register `ebx`.
* `arg2` maps to CPU register `esi`.
* `arg3` maps to CPU register `edi`.

This system call is not supported by all CPUs. It should only be used if the
"Get System Call Mechanism" system call indicates this is the right one to use.

### SYSENTER/SYSEXIT (Fast Intel) Mechanism

A system call can be invoked by executing the `SYSENTER` CPU instruction.

* `arg0` maps to CPU register `eax`.
* `arg1` maps to CPU register `ebx`.
* `arg2` maps to CPU register `esi`.
* `arg3` maps to CPU register `edi`.
* The return address must be set in the `ecx` CPU register.
* The user stack pointer must be set in the `ebp` CPU register.

This system call is not supported by all CPUs. It should only be used if the
"Get System Call Mechanism" system call indicates this is the right one to use.

## Error Numbers

(TODO bla bla bla)

* JINUE_EMORE 1
* JINUE_ENOMEM 2
* JINUE_ENOSYS 3
* JINUE_EINVAL 4
* JINUE_EAGAIN 5
* JINUE_EBADF 6
* JINUE_EIO 7
* JINUE_EPERM 8
* JINUE_E2BIG 9
* JINUE_ENOMSG 10

## System Call Reference

| Number  | Name                                    | Description                          |
|---------|-----------------------------------------|--------------------------------------|
| 0       | -                                       | Reserved                             |
| 1       | [GET_SYSCALL](get-syscall.md)           | Get System Call Mechanism            |
| 2       | [PUTC](putc.md)                         | Write Character to Console           |
| 3       | [PUTS](puts.md)                         | Write String to Console              |
| 4       | [CREATE_THREAD](create-thread.md)       | Create a thread                      |
| 5       | [YIELD_THREAD](yield-thread.md)         | Yield From or Destroy Current Thread |
| 6       | [SET_THREAD_LOCAL](set-thread-local.md) | Set Thread-Local Storage             |
| 7       | [GET_THREAD_LOCAL](get-thread-local.md) | Get Thread-Local Storage Address     |
| 8       | [GET_USER_MEMORY](get-user-memory.md)   | Get User Memory Map                  |
| 9       | [CREATE_IPC](create-ipc.md)             | Create IPC Endpoint                  |
| 10      | [RECEIVE](receive.md)                   | Receive Message                      |
| 11      | [REPLY](reply.md)                       | Reply to Message                     |
| 12-4095 | -                                       | Reserved                             |
| 4096+   | [SEND](send.md)                         | Send Message                         |

### Reserved Function Numbers

Any function marked as reserved returns -1 (in `arg0`) and sets error number
JINUE_ENOSYS (in `arg1`).
