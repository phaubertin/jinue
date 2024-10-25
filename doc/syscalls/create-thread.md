# CREATE_THREAD - Create a Thread

## Description

Create a new thread in a target process and bind it to a thread descriptor.

For this operation to succeed, the descriptor for the target process
must have the
[JINUE_PERM_CREATE_THREAD](../include/jinue/shared/asm/permissions.h) and
[JINUE_PERM_OPEN](../include/jinue/shared/asm/permissions.h) permissions.

## Arguments

Function number (`arg0`) is 4.

The descriptor number to bind to the new thread is set in `arg1`.

The descriptor number for the target process is set in `arg2`.

```
    +----------------------------------------------------------------+
    |                         function = 4                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                    thread descriptor number                    |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                    process descriptor number                   |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

## Return Value

On success, this function returns 0 (in `arg0`). On failure, this function
returns -1 and an error number is set (in `arg1`).

## Errors

* JINUE_EBADF if the specified descriptor is already in use.
* JINUE_EBADF if the target process descriptor is invalid, or does not refer to
a process, or is closed.
* JINUE_EPERM if the specified process descriptor does not have the permissions
to create a thread and bind a descriptor into the process.
