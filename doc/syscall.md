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

(TODO add a summary table)

### Get System Call Mechanism

#### Description

Get the best supported system call mechanism.

#### Arguments

Function number (`arg0`) is 1.

#### Return Value

Return value (`arg0`) identifies the system call mechanism:

* 0 for SYSENTER/SYSEXIT (Fast Intel) Mechanism
* 1 for SYSCALL/SYSRET (Fast AMD) Mechanism
* 2 for Interrupt-Based Mechanism

#### Errors

This function always succeeds.

#### Future Direction

This system call may be removed in the future, with the value it returns passed
in auxiliary vectors instead.

### Write Character to Console

#### Description

Write a single character to the console.

#### Arguments

Function number (`arg0`) is 2.

The character is passed in the lower eight bits of arg1.

```
    +----------------------------------------------------------------+
    |                         function = 2                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +------------------------------------------------+---------------+
    |                    reserved (0)                |   character   |  arg1
    +------------------------------------------------+---------------+
    31                                              8 7              0

    +----------------------------------------------------------------+
    |                         reserved (0)                           |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```
#### Return Value

No return value.

#### Errors

This function always succeeds.

#### Future Direction

A proper logging system call with log levels and a ring buffer in the kernel
will be implemented. Once this happens, this system call will be removed and
replaced.

### Write String to Console

#### Description

Write a character string to the console.

#### Arguments

Function number (`arg0`) is 3.

The pointer to the string is set in `arg1`.

The length of the string is passed in bits 19..8 of `arg3`. The string does not
have to be NUL-terminated.

```
    +----------------------------------------------------------------+
    |                         function = 3                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                       address of string                        |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         reserved (0)                           |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +-----------------------+------------------------+---------------+
    |     reserved (0)      |         length         |  reserved (0) |  arg3
    +-----------------------+------------------------+---------------+
    31                    20 19                     8 7              0
```

#### Return Value

No return value.

#### Errors

This function always succeeds.

#### Future Direction

A proper logging system call with log levels and a ring buffer in the kernel
will be implemented. Once this happens, this system call will be removed and/or
replaced.

### Create a Thread

#### Description

Create a new thread in the current process.

#### Arguments

Function number (`arg0`) is 4.

The address where code execution will start is set in `arg1`.

The value of the initial stack pointe is set in `arg2`.

```
    +----------------------------------------------------------------+
    |                         function = 4                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                   code entry point address                     |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                      user stack address                        |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

#### Return Value

On success, this function returns 0 (in `arg0`). On failure, this function
returns -1 and an error number is set (in `arg1`).

#### Errors

* JINUE_EINVAL the code entry point is set to a kernel address.
* JINUE_EINVAL the user stack address is set to a kernel address.
* JINUE_EAGAIN the thread could not be created because of needed resources.

#### Future Direction

Currently, this system call can only create a thread in the current process.
This will be changed so a thread can be created in another process, referred to
by a descriptor.

This system call will also be modified to bind the new thread to a descriptor
so other system call can refer to it e.g. to destroy it, join it, etc.

### Yield From or Destroy Current Thread

#### Description

Yield or destroy the current thread. Yielding relinquishes the CPU and allow
other ready threads to run.

#### Arguments

Function number (`arg0`) is 5.

If any bit in `arg1` is set (i.e. one), then the current thread is destroyed.
Otherwise, the current thread yields.

```
    +----------------------------------------------------------------+
    |                         function = 5                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +------------------------------------------------------------+---+
    |                         reserved (0)                       | D |  arg1
    +------------------------------------------------------------+---+
    31                                                          2 1  0

    +----------------------------------------------------------------+
    |                         reserved (0)                           |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

#### Return Value

No return value.

If destroying the current thread, this function does not return.

#### Errors

This function always succeeds.

#### Future Direction

This system call will be split into two system calls, one that yields and one
that destroys the current thread.

The current system call only affects the current thread. This may be changed to
allow another thread to be destroyed, in which case the thread would be
specified using a descriptor.

### Set Thread-Local Storage

#### Description

Set the address and size of the thread-local storage for the current thread.

A thread can then retrieve the address of the thead-local storage by calling the
Get Thread Local Storage Address system call.

#### Arguments

Function number (`arg0`) is 6.

The address of (i.e. pointer to) the thread-local storage is set in `arg1`.

The size, in bytes, of the thread-local storage is set in `arg2`.

```
    +----------------------------------------------------------------+
    |                         function = 6                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                 thread-local storage address                   |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                   thread-local storage size                    |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

#### Return Value

On success, this function returns 0 (in `arg0`). On failure, it returns -1 and
an error number is set (in `arg1`).

#### Errors

* JINUE_EINVAL if any part of the region set with this function belongs to the
kernel.

#### Future Direction

The current system call can only set the thread-local storage for the current
thread. This may be changed to allow setting the thread-local storage for
another thread, which would be specified using a descriptor.

Support will also be implemented in the kernel for an architecture-dependent
mechanism to access thread-local storage that is faster than calling the
Get Thread Local Storage Address system call. On x86, this will likely mean
dedicating an entry to thread-local storage in the GDT.

### Get Thread Local Storage Address

#### Description

Get the address of the thread-local storage for the current thread.

A thread can set the address and size of its thread-local storage by calling the
Set Thread-Local Storage system call.

#### Arguments

Function number (`arg0`) is 7.

#### Return Value

This function returns the address of the thread-local storage in `arg0`. If this
has not been set for the current thread by a previous call to the Set
Thread-Local Storage system call, then the return value is zero (i.e. the NULL
pointer in C).

Note that, since this function returns a pointer, it does not follow the usual
convention whereby a negative return value indicates a failure.

#### Errors

This function always succeeds.

#### Future Direction

Support will be implemented in the kernel for an architecture-dependent
mechanism to access thread-local storage that is faster than calling this
function. On x86, this will likely mean dedicating an entry to thread-local
storage in the GDT. Once this happens, this function may or may not be
deprecated.

### Get Memory Map

#### Description

This function writes the system memory map in a buffer provided by the caller.

The format is the same as the PC BIOS interrupt 15 hexadecimal, function ax=e820
hexadecimal.

#### Arguments

Function number (`arg0`) is 8.

A pointer to the destination buffer is set in `arg2`. The size of the buffer is
set in bits 31..20 of `arg3`.

```
    +----------------------------------------------------------------+
    |                         function = 8                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                         reserved (0)                           |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                        buffer address                          |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +-----------------------+----------------------------------------+
    |     buffer size       |              reserved (0)              |  arg3
    +-----------------------+----------------------------------------+
    31                    20 19                                      0
```

#### Return Value

On success, this function returns 0 (in `arg0`). On failure, it returns -1 and
an error number is set (in `arg1`).

#### Errors

* JINUE_EINVAL if any part of the destination buffer belongs to the kernel.
* JINUE_EINVAL if the output buffer is larger that 2048 bytes.
* JINUE_E2BIG if the output buffer is too small to fit the result.

#### Future Direction

This function provides no information about memory used by the kernel. This will
need to be remediated for this system call to be useful.

The format of the memory map may be changed to one that is architecture
indpendent.

Mapping of the argument may be changed to something more intuitive. (The
rationale behind the current layout is that is is consistent with argument
mapping for the IPC system calls.)

The 2048 bytes restriction on the output buffer size will be eliminated.

### Create IPC Endpoint

#### Description

Create a new IPC endpoint.

#### Arguments

Function number (`arg0`) is 9.

Flags are set in `arg1` as follow:

* If bit 0 is set, then the created IPC endpoint is a "system" endpoint. (See
Future Direction).
* If bit 1 is set, then, instead if creating a new IPC endpoint, it retrieves
the special-purpose endpoint that allows the process to communicate with its
creating parent. This is useful for service discovery.

```
    +----------------------------------------------------------------+
    |                         function = 9                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                            flags                               |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         reserved (0)                           |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

#### Return Value

On success, this function returns the descriptor number for the new IPC endpoint
(in `arg0`). On failure, it returns -1 and an error number is set (in `arg1`).

#### Errors

* JINUE_EAGAIN if a descriptor number could not be allocated.
* JINUE_EAGAIN if the IPC endpoint could not be created because of insufficient
resources.

#### Future Direction

This function will be modified so the descriptor number is passed as an argument
by the caller instead of being chosen by the microkernel and returned.

A "system" IPC endpoint is currently no different than a regular IPC endpoint.
The original intent behind the system flag in arguments was to enforce special
access control rules for "system" endpoints. However, the flag will likely be
deprecated instead.

### Receive Message

#### Description

Receive a message from an IPC endpoint. If no message is available, this call
blocks until one becomes available.

#### Arguments

Function number (`arg0`) is 10.

The descriptor that references the IPC endpoint is passed in `arg1`.

A pointer to the receive buffer is passed in `arg2` and the size, in bytes, of
the receive buffer is passed in bits 31..20 of `arg3`.

```
    +----------------------------------------------------------------+
    |                          function = 10                         |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                    IPC endpoint descriptor                     |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                        buffer address                          |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +-----------------------+----------------------------------------+
    |     buffer size       |              reserved (0)              |  arg3
    +-----------------------+----------------------------------------+
    31                    20 19                                      0
```

#### Return Value

On success:

* `arg0` is set to the function number specified by the sender. This function
number is guaranteed to be positive.
* `arg1` contains the cookie set on the descriptor used by the sender to send
the message.
* `arg2` contains the adress of the receive buffer (i.e. `arg2` is preserved by
this system call).
* Bits 31..20 of `arg3` contains the *sender's* message buffer size. The reply
message must fit within this size.
* Bits 19..8 of `arg3` is the size of the message, in bytes.

On failure, this function sets `arg0` to -1 and an error number in `arg1`.

```
    +----------------------------------------------------------------+
    |                         function (or -1)                       |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                    descriptor cookie (or error)                |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                        buffer address                          |  arg2
    +----------------------------------------------------------------+
    31                                                               0
    
    +-----------------------+------------------------+---------------+
    |  sender buffer size   |    message data size   |  reserved (0) |  arg3
    +-----------------------+------------------------+---------------+
    31                    20 19                     8 7              0
```

#### Errors

* JINUE_EBADF if the specified descriptor is invalid or does not refer to an
IPC endpoint.
* JINUE_EIO if the specified descriptor is closed or the IPC endpoint no longer
exists.
* JINUE_EINVAL if any part of the receiver buffer belongs to the kernel.
* JINUE_EINVAL if the receive buffer is larger than 2048 bytes.
* JINUE_E2BIG if a message was available but it was too big for the receive
buffer.
* JINUE_EPERM if the process to which the calling thread belongs is not the
owner of the IPC endpoint.

#### Future Direction

This function will be modified to allow receiving descriptors from the sender.

A combined reply/receive system call will be added to allow the receiver thread
to atomically send a reply to the current message and wait for the next one.

A non-blocking version of this system call that would return immediately if no
message is available may also be added.

The 2048 bytes restriction on the receive buffer size may be eliminated.

Ownership of an IPC endpoint by a process may be replaced by a receive
permission that can be delegated to another process.

### Reply to Message

#### Description

Reply to the current message. This function should be called after calling the
Receive Message system call in order to send the reply and complete processing
of the message.

#### Arguments

Function number (`arg0`) is 11.

The descriptor that references the IPC endpoint is passed in `arg1`.

A pointer to the buffer containing the reply message is passed in `arg2` and the
size, in bytes, of the buffer is passed in bits 31..20 of `arg3`.

The length of the reply data length is set in bits 19..8 of `arg3` (see Future
Direction).

```
    +----------------------------------------------------------------+
    |                        function = 11                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                         reserved (0)                           |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                        buffer address                          |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +-----------------------+------------------------+---------------+
    |     buffer size       |     reply data size    |  reserved (0) |  arg3
    +-----------------------+------------------------+---------------+
    31                    20 19                     8 7              0
```

#### Return Value

On success, this function returns 0 (in `arg0`). On failure, it returns -1 and
an error number is set (in `arg1`).

#### Errors

* JINUE_ENOMSG if there is no current message, i.e. if no message was received
using the Receive Message system call.
* JINUE_EINVAL if the reply buffer is larger than 2048 bytes.
* JINUE_EINVAL if the reply data length is larger than the reply buffer size.
* JINUE_EINVAL if any part of the reply buffer belongs to the kernel.
* JINUE_E2BIG if the reply message is too big for the peer's buffer size. (This
buffer size is available in bits 31..20 of `arg3` in the return value of the
Receive Message system call.)

#### Future Direction

This function will be modified to allow sending descriptors as part of the
reply message. (This is why there are separate arguments for the buffer size and
the reply data length.)

A combined reply/receive system call will be added to allow the receiver thread
to atomically send a reply to the current message and wait for the next one.

### Send Message

#### Description

Send a message to an IPC endpoint. This call blocks until the message is
received and replied to.

#### Arguments

The function number, which must be at least 4096, is set in `arg0`. The function
number is passed to the receiving thread along with the message.

The descriptor that references the IPC endpoint is passed in `arg1`.

A pointer to the message buffer is passed in `arg2` and the size, in bytes, of
that buffer is passed in bits 31..20 of `arg3`. The same buffer is used for the
message being sent and for receiving the reply, and it must be large enough for
both.

The data length of the message is set in bits 19..8 of `arg3`.

```
    +----------------------------------------------------------------+
    |                        function number                         |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                    IPC endpoint descriptor                     |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                        buffer address                          |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +-----------------------+------------------------+---------------+
    |     buffer size       |   message data size    |  reserved (0) |  arg3
    +-----------------------+------------------------+---------------+
    31                    20 19                     8 7              0
```

#### Return Value

On success, the return value set in `arg0` is zero and the size of the reply
message is set in bits 19..8 of `arg3`.

On failure, the return value set in `arg0` is -1 and an error number is set in
`arg1`.
    
```
    +----------------------------------------------------------------+
    |                            0 (or -1)                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                          0 (or error)                          |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                          reserved (0)                          |  arg2
    +----------------------------------------------------------------+
    31                                                               0
    
    +-----------------------+------------------------+---------------+
    |      reserved (0)     |    reply data size     |  reserved (0) |  arg3
    +-----------------------+------------------------+---------------+
    31                    20 19                     8 7              0
```

#### Errors

* JINUE_EBADF if the specified descriptor is invalid or does not refer to an
IPC endpoint.
* JINUE_EIO if the specified descriptor is closed or the IPC endpoint no longer
exists.
* JINUE_EINVAL if the buffer is larger than 2048 bytes.
* JINUE_EINVAL if the message data length is larger than the buffer size.
* JINUE_EINVAL if any part of the message buffer belongs to the kernel.
* JINUE_E2BIG if the receiving thread attempted to send a reply that was too big
for the buffer size.

#### Future Direction

This function will be modified to allow sending descriptors as part of the
message.

A non-blocking version of this system call that would return immediately if no
thread is waiting to receive the message may also be added.

The 2048 bytes restriction on the buffer size may be eliminated.
