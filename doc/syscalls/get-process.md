# GET_PROCESS - Get Descriptor for Current Process

## Description

Get a descriptor that represents the current process.

## Arguments

Function number (`arg0`) is 14.

```
    +----------------------------------------------------------------+
    |                         function = 14                          |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                         reserved (0)                           |  arg1
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

On success, this function returns the descriptor number (in `arg0`). On failure,
it returns -1 and an error number is set (in `arg1`).

## Errors

* JINUE_EAGAIN if a descriptor number could not be allocated.

## Future Direction

Currently, the kernel allocates a new descriptor number to represent the
process. This will be changed so the descriptor is passed as an argument and
the kernel binds that descriptor to the current process.
