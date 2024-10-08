# DESTROY - Destroy a Kernel Object

## Description

Close a descriptor and destroy the kernel object to which it refers.

In order to use this function, the owner descriptor for the resource must be
specified. The owner descriptor is the descriptor that was specified in the
call to the function that created the resource (e.g. CREATE_IPC).

## Arguments

Function number (`arg0`) is 18.

The descriptor number is set in `arg1`.

```
    +----------------------------------------------------------------+
    |                        function = 18                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                      descriptor number                         |  arg1
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

On success, this function returns 0 (in `arg0`). On failure, this function
returns -1 and an error number is set (in `arg1`).

## Errors

* JINUE_EBADF if the specified target process descriptor is invalid or is
already closed.
* JINUE_EPERM if the specified descriptor is not the owner descriptor.
