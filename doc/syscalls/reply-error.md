# REPLY_ERROR - Reply to Message with an Error

## Description

Reply to the current message with an error code. This function should be
called after calling the [RECEIVE](receive.md) system call in order to send
an error reply and complete processing of the message.

When this function is called, the call to [SEND](send.md) that caused the
current message to be received fails with error number
[JINUE_EPROTO](../../include/jinue/shared/asm/errno.h) and the error code
specified to this function is part of the return value. See [SEND](send.md)
for detail.

## Arguments

Function number (`arg0`) is 22.

The error code is set in `arg1`

```
    +----------------------------------------------------------------+
    |                        function = 22                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                          error code                            |  arg1
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

## Return Value

On success, this function returns 0 (in `arg0`). On failure, it returns -1 and
an error number is set (in `arg1`).

## Errors

* JINUE_ENOMSG if there is no current message, which means either:
    * [RECEIVE](receive.md) was not called; or
    * [RECEIVE](receive.md) was called but did not return successfully; or
    * This function or [REPLY](reply.md) was already called at least once since
      the last call to [RECEIVE](receive.md).
