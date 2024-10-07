# RECEIVE - Receive Message

## Description

Receive a message from an IPC endpoint. If no message is available, this call
blocks until one becomes available.

On a successful receive, this function sets the following members in the
[jinue_message_t structure](../../include/jinue/shared/ipc.h) passed as
argument:

* `recv_function` is set to the function number that was specified by the sender
  when it invoked [SEND](send.md).
* `recv_cookie` is set to the cookie associated with the descriptor specified by
  the sender as part of the arguments to [SEND](send.md).
* `reply_max_size` is set to the maximum size of the reply that can be sent back
  when [REPLY](reply.md)ing to the sender.

## Arguments

Function number (`arg0`) is 10.

The descriptor that references the IPC endpoint is passed in `arg1`.

A pointer to a [jinue_message_t structure](../../include/jinue/shared/ipc.h)
is passed in `arg2`. In this structure, the receive buffers must be set to
where the message is to be written.

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

* JINUE_EBADF if the specified descriptor is invalid, or does not refer to an
IPC endpoint, or is closed.
* JINUE_EPERM if the descriptor does not have receive permissions on the IPC
endpoint.
* JINUE_EIO if the IPC endpoint no longer exists.
* JINUE_E2BIG if a message was available but it was too large for the receive
buffer.
* JINUE_EINVAL in any of the following situations:
    * If any part of any of the receive buffers belongs to the kernel.
    * If the receive buffers array has more than 256 elements.
    * If any of the receive buffers is larger than 64 MB.

## Future Direction

This function will be modified to allow receiving descriptors from the sender as
part of the message.

A combined reply/receive system call will be added to allow the receiver thread
to atomically send a reply to the current message and wait for the next one.

A non-blocking version of this system call that would return immediately if no
message is available may also be added.

Ownership of an IPC endpoint by a process may be replaced by a receive
permission that can be delegated to another process.
