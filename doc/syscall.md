# System Call Interface

## Argument Registers

During a system call, information is passed back and forth between user space
and the microkernel through four pointer-sized logical registers called `arg0`
to `arg3`.

When invoking a system call, `arg0` contains the system call number and `arg1`
to `arg3` contain arguments for the call:

* System call numbers 0 to 4095 included are reserved by the microkernel.
* System call numbers 4096 and up all identify the Send Message system call. The
system call number is passed to the recipient as part of the message.

TODO arguments are not necessarily in arg1, then arg2, then arg3

On return from a system call, the contents of arg0 to arg3 depends on the
system call number. Most *but not all* system calls follow the following
convention:

* `arg0` contains a return value, which should be cast as a signed integer (C
type `int`). If the value is positive (including zero), then the call was
successful. A non-zero negative value indicates an error has occurred.
* If the call failed, as indicated by the value in `arg0`, `arg1` contains the
error number. Otherwise, it is set to zero.
* `arg2` and `arg3` are reserved and should be ignored.

## System Call Mechanisms

The microkernel supports the following mechanisms to invoke system calls. Not
all mechanisms are available on all CPUs. On initialization, before invoking any
other system call, an application should call the "Get System Call Mechanism"
system call to determine the best supported system call mechanism. This call of
the "Get System Call Mechanism" system call should be done using the
interrupt-based mechanism, which is the only one garanteed to be supported by
all CPUs.

(TODO should we pass this through the auxiliary vectors instead?)

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

### Get System Call Mechanism

System call number: 1

TODO

#### Arguments

```
    +----------------------------------------------------------------+
    |                        msgFunction = 1                         |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

#### Return Value

```
    +----------------------------------------------------------------+
    |                  System call mechanism number                  |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

System call mechanism number:

* 0 Fast Intel
* 1 Fast AMD
* 2 Interrupt-based

### Write Character to Console

System call number: 2

TODO

#### Arguments

```
    +----------------------------------------------------------------+
    |                        msgFunction = 2                         |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +------------------------------------------------+---------------+
    |                    Reserved (0)                |   character   |  arg1
    +------------------------------------------------+---------------+
    31                                              8 7              0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

#### Return Value

```
    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

### Write String to Console

System call number: 3

TODO

#### Arguments

```
    +----------------------------------------------------------------+
    |                        msgFunction = 3                         |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                       Pointer on string                        |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +-----------------------+------------------------+---------------+
    |     Reserved (0)      |      msgDataSize       |  Reserved (0) |  arg3
    +-----------------------+------------------------+---------------+
    31                    20 19                     8 7              0
```

#### Return Value

```
    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

### Create a Thread

System call number: 4

TODO

#### Arguments

```
    +----------------------------------------------------------------+
    |                        msgFunction = 4                         |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                   Code entry point address                     |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                      User stack address                        |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

#### Return Value

```
    +----------------------------------------------------------------+
    |                           0 (or -1)                            |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                      0 (or error number)                       |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

### Yield From or Destroy Current Thread

System call number: 5

TODO

#### Arguments

```
    +----------------------------------------------------------------+
    |                        msgFunction = 5                         |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +------------------------------------------------------------+---+
    |                         Reserved (0)                       | D |  arg1
    +------------------------------------------------------------+---+
    31                                                          2 1  0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

#### Return Value

```
    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

D: destroy or not

### Set Thread Local Storage Address

System call number: 6

TODO

#### Arguments

```
    +----------------------------------------------------------------+
    |                        msgFunction = 6                         |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                 Thread local storage address                   |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                   Thread local storage size                    |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

#### Return Value

```
    +----------------------------------------------------------------+
    |                           0 (or -1)                            |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                      0 (or error number)                       |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

### Get Thread Local Storage Address

System call number: 7

TODO

#### Arguments

```
    +----------------------------------------------------------------+
    |                        msgFunction = 7                         |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

#### Return Value

```
    +----------------------------------------------------------------+
    |                 Thread local storage address                   |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

### Get Memory Map

System call number: 8

TODO

#### Arguments

```
    +----------------------------------------------------------------+
    |                        msgFunction = 8                         |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                        Buffer pointer                          |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +-----------------------+----------------------------------------+
    |     Buffer size       |              Reserved (0)              |  arg3
    +-----------------------+----------------------------------------+
    31                    20 19                                      0
```

#### Return Value

```
    +----------------------------------------------------------------+
    |                           0 (or -1)                            |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                      0 (or error number)                       |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

### Create IPC Endpoint

System call number: 9

TODO

#### Arguments

```
    +----------------------------------------------------------------+
    |                        msgFunction = 9                         |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                         Flags (TODO)                           |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

#### Return Value

```
    +----------------------------------------------------------------+
    |                    descriptor number (or -1)                   |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                      0 (or error number)                       |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

### Receive Message

System call number: 10

TODO

#### Arguments

Receive system call arguments (passed in registers):

```
    +----------------------------------------------------------------+
    |                         msgFunction = 10                       |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +-------------------------------+--------------------------------+
    |         Reserved (0)          |          msgRecvDesc           |  arg1
    +-------------------------------+--------------------------------+
    31                             ? ?                                0

    +----------------------------------------------------------------+
    |                            msgPtr                              |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +-----------------------+------------------------+---------------+
    |     msgTotalSize      |       Reserved (0)     |   msgDescN    |  arg3
    +-----------------------+------------------------+---------------+
    31                    20 19                     8 7              0
```

Where:

    msgFunction     is the system call number for RECEIVE.
    msgRecvDesc     is the descriptor for the door from which to receive a
                    message. It must be the owning descriptor for this door.
    msgPtr          is address of the start of the buffer in which to receive
                    the message.
    msgTotalSize    is the total size of the receive buffer, in bytes.
    msgDescN        is the number of descriptors. All descriptor must be
                    specified with the JINUE_DESC_RECEIVE attribute flag.

#### Return Value
             
When the RECEIVE or REPLY/RECEIVE system call returns, the arguments provided by
the microkernel are as follow:

```
    +----------------------------------------------------------------+
    |                        msgFunction (or -1)                     |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                         msgCookie (or error)                   |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                            msgPtr                              |  arg2
    +----------------------------------------------------------------+
    31                                                               0
    
    +-----------------------+------------------------+---------------+
    |     msgTotalSize      |      msgDataSize       |   msgDescN    |  arg3
    +-----------------------+------------------------+---------------+
    31                    20 19                     8 7              0
```
    
Where:

    msgFunction     is the function number (or -1 if the call to the receive
                    system call fails).
    msgCookie       is the message cookie, as stored by the recipient into the
                    sender's descriptor (or the error number if the call to the
                    receive system call fails).
    msgPtr          is the address of the start of the message buffer (in the
                    recipient's address space).
    msgTotalSize    is the total size of the *sender's* buffer, in bytes. This
                    allows the receiver to know the maximum size allowed for the
                    reply message (and to return an error if it is insufficient).
    msgDataSize     is the size of the message data, in bytes.
    msgDescN        is the number of descriptors.

### Reply to Message

System call number: 11

TODO

#### Arguments

When replying, the receiver sets the message arguments as follow:

```
    +----------------------------------------------------------------+
    |                       msgFunction = 11                         |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                         Reserved (0)                           |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                           msgPtr                               |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +-----------------------+------------------------+---------------+
    |     msgTotalSize      |      msgDataSize       |   msgDescN    |  arg3
    +-----------------------+------------------------+---------------+
    31                    20 19                     8 7              0
```

Where:

    msgFunction     is the system call number for REPLY or REPLY/RECEIVE
    msgPtr          is the address of the start of the reply message buffer. It
                    may or may not be the buffer where the message being replied
                    to was received. For a combined REPLY/RECEIVE, this is also
                    the buffer in which to receive the next message.
    msgTotalSize    is the total size of the receive buffer, in bytes. This
                    field is only relevant for a combined REPLY/RECEIVE).
    msgDataSize     is the size of the message data, in bytes.
    msgDescN        is the number of descriptors.

#### Return Value

TODO

### Send Message

System call number: 4096 and up

TODO

#### Arguments

Send message arguments (passed in registers):

```
    +----------------------------------------------------------------+
    |                          msgFunction                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +-------------------------------+--------------------------------+
    |           Reserved            |        msgTargetDesc           |  arg1
    +-------------------------------+--------------------------------+
    31                             ? ?                                0

    +----------------------------------------------------------------+
    |                            msgPtr                              |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +-----------------------+------------------------+---------------+
    |     msgTotalSize      |      msgDataSize       |   msgDescN    |  arg3
    +-----------------------+------------------------+---------------+
    31                    20 19                     8 7              0
```

Where:
    
    msgFunction     is the function or system call number.
    msgTargetDesc   is the descriptor for the target of the call (door, thread).
    msgPtr          is address of the start of the message buffer.
    msgTotalSize    is the total size of the buffer, in bytes.
    msgDataSize     is the size of the message data, in bytes.
    msgDescN        is the number of descriptors.

#### Return Value
    
When the send primitive returns to the original caller, the arguments provided
by the microkernel are as follow:

```
    +----------------------------------------------------------------+
    |                            msgRetVal                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                            msgErrno                            |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                            Reserved                            |  arg2
    +----------------------------------------------------------------+
    31                                                               0
    
    +-----------------------+------------------------+---------------+
    |       Reserved        |      msgDataSize       |   msgDescN    |  arg3
    +-----------------------+------------------------+---------------+
    31                    20 19                     8 7              0
```    

Where:

    msgRetVal       is the first 32-bit value at the start of the message buffer
                    if msgDataSize is at least 4 bytes, and zero otherwise. By
                    convention, this is typically used to provide a return value.
    msgErrno        is the second 32-bit value from the start of the message
                    buffer if msgDataSize is at least 8 bytes, and zero
                    otherwise. By convention, this is typically used to provide
                    an error code (or zero if the call was successful).
    msgDataSize     is the size of the reply message data, in bytes.
    msgDescN        is the number of descriptors in the reply.
