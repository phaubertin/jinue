# DUP - Duplicate a Descriptor

## Description

Create a copy of a descriptor from the current process in a target process.

For this operation to succeed, the descriptor for the target process
must have the [JINUE_PERM_OPEN](../../include/jinue/shared/asm/permissions.h)
permission.

A owner descriptor cannot be duplicated with this (or any) function. The owner
descriptor for a given kernel resource is the original descriptor to which the
resource was bound when it was created through the relevant system call (e.g.
[CREATE_ENDPOINT](create-endpoint) for an IPC endpoint). While a owner
descriptor cannot be duplicated using this function, the [MINT](mint.md)
function can be used instead to create a new descriptor that refers to the same
kernel resource and has customized permissions.

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
* JINUE_EPERM if the specified process descriptor does not have the permission
to bind a descriptor into the process.
