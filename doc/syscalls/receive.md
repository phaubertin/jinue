# RECEIVE - Receive Message

## Description

Receive a message from an IPC endpoint. If no message is available, this call
blocks until one becomes available.

## Arguments

Function number (`arg0`) is 10.

The descriptor that references the IPC endpoint is passed in `arg1`.

A pointer to a jinue_message_t structure is passed in `arg2`. In this structure,
the receive buffers should be set to the where the message will be written.

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
    |                    receive message address                     |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                          reserved (0)                          |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

## Return Value

On success, the return value set in `arg0` is the size of the message data in
bytes. On failure, the return value set in `arg0` is -1 and an error number is
set in `arg1`.

## Errors

* JINUE_EBADF if the specified descriptor is invalid or does not refer to an
IPC endpoint.
* JINUE_EIO if the specified descriptor is closed or the IPC endpoint no longer
exists.
* JINUE_EINVAL if any part of the receiver buffer belongs to the kernel.
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

Ownership of an IPC endpoint by a process may be replaced by a receive
permission that can be delegated to another process.
