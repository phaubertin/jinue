# REPLY - Reply to Message

## Description

Reply to the current message. This function should be called after calling the
[RECEIVE](receive.md) system call in order to send the reply and complete
processing of the message.

## Arguments

Function number (`arg0`) is 11.

A pointer to a [jinue_message_t structure](../../include/jinue/shared/ipc.h)
is passed in `arg2`. In this structure, the send buffers must be set to the
reply data.

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

* JINUE_ENOMSG if there is no current message, which means either:
    * [RECEIVE](receive.md) was not called; or
    * [RECEIVE](receive.md) was called but did not return successfully; or
    * This function was already called at least once since the last call to
      [RECEIVE](receive.md).
* JINUE_E2BIG if the reply message is too big for the sender's receive buffer
size.
* JINUE_EINVAL in any of the following situations:
    * If the total length of the reply is larger than 2048 bytes.
    * If any part of any of the send buffers belongs to the kernel.
    * If the send buffers array has more than 256 elements.
    * If any of the send buffers is larger than 64 MB.

## Future Direction

This function will be modified to allow sending descriptors as part of the
reply message.

A combined reply/receive system call will be added to allow the receiver thread
to atomically send a reply to the current message and wait for the next one.
