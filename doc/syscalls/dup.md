# DUP - Duplicate a Descriptor

## Description

Create a copy of a descriptor from the current process in a target process.

A owner descriptor cannot be duplicated with this (or any) function. A owner
descriptor is the descriptor that was specified in the call to the function
that created a kernel resource (e.g. [CREATE_IPC](create-ipc.md)) and that
can be used to [mint](mint.md) other descriptors to that resource.

## Arguments

Function number (`arg0`) is 16.

The descriptor number for the target process is set in `arg1`.

The source descriptor is set in `arg2` and the destination descriptor is set in
`arg3`.

```
    +----------------------------------------------------------------+
    |                        function = 16                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                           process                              |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                             src                                |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                             dest                               |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

## Return Value

On success, this function returns 0 (in `arg0`). On failure, this function
returns -1 and an error number is set (in `arg1`).

## Errors

* JINUE_EBADF if the specified target process descriptor is invalid, or does
not refer to a process, or is closed.
* JINUE_EBADF if the specified source descriptor is invalid, or is a owner
descriptor, or is closed.
* JINUE_EBADF if the specified destination descriptor is already in use.
