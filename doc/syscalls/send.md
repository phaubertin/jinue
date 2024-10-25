# SEND - Send Message

## Description

Send a message to an IPC endpoint. This call blocks until the message is
received and replied to.

For this operation to succeed, the IPC endpoint descriptor must have the
[JINUE_PERM_SEND](../include/jinue/shared/asm/permissions.h) permission.

## Arguments

The function number, which must be at least 4096, is set in `arg0`. The function
number is passed to the receiving thread along with the message.

The descriptor that references the IPC endpoint is passed in `arg1`. 

A pointer to a [jinue_message_t structure](../../include/jinue/shared/ipc.h)
is passed in `arg2`. In this structure, the send buffers must be set to the
message data to be sent while the receive buffers describe where to write the
reply.

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
    |                        message address                         |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                          reserved (0)                          |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

## Return Value

On success, the return value set in `arg0` is the size of the reply in bytes. On
failure, the return value set in `arg0` is -1 and an error number is set in
`arg1`.
    
## Errors

* JINUE_EBADF if the specified descriptor is invalid, or does not refer to an
IPC endpoint, or is closed.
* JINUE_EPERM if the descriptor does not have send permissions on the IPC
endpoint.
* JINUE_EIO if the IPC endpoint no longer exists.
* JINUE_E2BIG if a receiving thread attempted to receive the message but it was
too large for its receive buffer(s).
* JINUE_EINVAL in any of the following situations:
    * If the total length of the message is larger than 2048 bytes.
    * If any part of any of the send and/or receive buffers belongs to the kernel.
    * If the send and/or receive buffers array have more than 256 elements.
    * If any of the receive buffers is larger than 64 MB.

## Future Direction

This function will be modified to allow sending descriptors as part of the
message and receive descriptors as part of the reply.

A non-blocking version of this system call that would return immediately if no
thread is waiting to receive the message may also be added.
