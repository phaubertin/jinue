# CREATE_THREAD - Create a Thread

## Description

Create a new thread in a target process.

## Arguments

Function number (`arg0`) is 4.

The descriptor number to bind to the new thread is set in `arg1`.

The descriptor number for the target process is set in `arg2`. This descriptor
must have the necessary permissions to create a thread in the target process
(JINUE_PERM_CREATE_THREAD) and to bind a descriptor in the target process
(JINUE_PERM_OPEN).


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
* JINUE_EPERM if the target process descriptor does not have the needed
permissions.
