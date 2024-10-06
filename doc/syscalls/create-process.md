# CREATE_PROCESS - Create a Process

## Description

Create a new process.

## Arguments

Function number (`arg0`) is 14.

The descriptor number to bind to the new process is set in `arg1`.

```
    +----------------------------------------------------------------+
    |                         function = 14                          |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                       descriptor number                        |  arg1
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

* JINUE_EBADF if the specified descriptor is already in use.
* JINUE_EAGAIN if the process could not be created because of needed resources.
