# DUP - Duplicate a Descriptor

## Description

Create a copy of a descriptor from the current process in a target process.

## Arguments

Function number (`arg0`) is 16.

The descriptor number for the target process is set in `arg1`.

The source descriptor is set in `arg2` and the destination descriptor is set in
`arg3`.

```
    +----------------------------------------------------------------+
    |                         function = 4                           |  arg0
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
* JINUE_EBADF if the specified source descriptor is invalid or is closed.
* JINUE_EBADF if the specified destination descriptor is already in use.
