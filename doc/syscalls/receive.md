# RECEIVE - Receive Message

## Description

Receive a message from an IPC endpoint. If no message is available, this call
blocks until one becomes available.

## Arguments

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

## Return Value

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

## Errors

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

## Future Direction

This function will be modified to allow receiving descriptors from the sender.

A combined reply/receive system call will be added to allow the receiver thread
to atomically send a reply to the current message and wait for the next one.

A non-blocking version of this system call that would return immediately if no
message is available may also be added.

The 2048 bytes restriction on the receive buffer size may be eliminated.

Ownership of an IPC endpoint by a process may be replaced by a receive
permission that can be delegated to another process.
