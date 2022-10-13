# REPLY - Reply to Message

## Description

Reply to the current message. This function should be called after calling the
Receive Message system call in order to send the reply and complete processing
of the message.

## Arguments

Function number (`arg0`) is 11.

The descriptor that references the IPC endpoint is passed in `arg1`.

A pointer to the buffer containing the reply message is passed in `arg2` and the
size, in bytes, of the buffer is passed in bits 31..20 of `arg3`.

The length of the reply data length is set in bits 19..8 of `arg3` (see
[Future Direction](#future-direction)).

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

## Return Value

On success, this function returns 0 (in `arg0`). On failure, it returns -1 and
an error number is set (in `arg1`).

## Errors

* JINUE_ENOMSG if there is no current message, i.e. if no message was received
using the Receive Message system call.
* JINUE_EINVAL if the reply buffer is larger than 2048 bytes.
* JINUE_EINVAL if the reply data length is larger than the reply buffer size.
* JINUE_EINVAL if any part of the reply buffer belongs to the kernel.
* JINUE_E2BIG if the reply message is too big for the peer's buffer size. (This
buffer size is available in bits 31..20 of `arg3` in the return value of the
Receive Message system call.)

## Future Direction

This function will be modified to allow sending descriptors as part of the
reply message. (This is why there are separate arguments for the buffer size and
the reply data length.)

A combined reply/receive system call will be added to allow the receiver thread
to atomically send a reply to the current message and wait for the next one.
