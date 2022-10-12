# SEND - Send Message

## Description

Send a message to an IPC endpoint. This call blocks until the message is
received and replied to.

## Arguments

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

## Return Value

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

## Errors

* JINUE_EBADF if the specified descriptor is invalid or does not refer to an
IPC endpoint.
* JINUE_EIO if the specified descriptor is closed or the IPC endpoint no longer
exists.
* JINUE_EINVAL if the buffer is larger than 2048 bytes.
* JINUE_EINVAL if the message data length is larger than the buffer size.
* JINUE_EINVAL if any part of the message buffer belongs to the kernel.
* JINUE_E2BIG if the receiving thread attempted to send a reply that was too big
for the buffer size.

## Future Direction

This function will be modified to allow sending descriptors as part of the
message.

A non-blocking version of this system call that would return immediately if no
thread is waiting to receive the message may also be added.

The 2048 bytes restriction on the buffer size may be eliminated.
