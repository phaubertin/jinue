# SEND - Send Message

## Description

Send a message to an IPC endpoint. This call blocks until the message is
received and replied to.

## Arguments

The function number, which must be at least 4096, is set in `arg0`. The function
number is passed to the receiving thread along with the message.

The descriptor that references the IPC endpoint is passed in `arg1`. 

A pointer to a jinue_message_t structure is passed in `arg2`. In this structure,
the send buffers should be set to the message data to be sent while the receive
buffers need to describe where to write the reply.

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

* JINUE_EBADF if the specified descriptor is invalid or does not refer to an
IPC endpoint.
* JINUE_EIO if the specified descriptor is closed or the IPC endpoint no longer
exists.
* JINUE_EINVAL if the total length of the message is larger than 2048 bytes.
* JINUE_EINVAL if any part of the message buffer belongs to the kernel.
* JINUE_E2BIG if the receiving thread attempted to send a reply that was too big
for the buffer size.

## Future Direction

This function will be modified to allow sending descriptors as part of the
message.

A non-blocking version of this system call that would return immediately if no
thread is waiting to receive the message may also be added.
