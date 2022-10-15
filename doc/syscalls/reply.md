# REPLY - Reply to Message

## Description

Reply to the current message. This function should be called after calling the
Receive Message system call in order to send the reply and complete processing
of the message.

## Arguments

Function number (`arg0`) is 11.

A pointer to a jinue_message_t structure is passed in `arg2`. In this structure,
the send buffers should be set to the reply data.

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
    |                    reply message address                       |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                          reserved (0)                          |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

## Return Value

On success, this function returns 0 (in `arg0`). On failure, it returns -1 and
an error number is set (in `arg1`).

## Errors

* JINUE_ENOMSG if there is no current message, i.e. if no message was received
using the Receive Message system call.
* JINUE_EINVAL if any part of the reply buffer belongs to the kernel.
* JINUE_E2BIG if the reply message is too big for the sender's receive buffer
size.

## Future Direction

This function will be modified to allow sending descriptors as part of the
reply message. (This is why there are separate arguments for the buffer size and
the reply data length.)

A combined reply/receive system call will be added to allow the receiver thread
to atomically send a reply to the current message and wait for the next one.
