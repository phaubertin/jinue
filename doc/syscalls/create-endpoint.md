# CREATE_ENDPOINT - Create IPC Endpoint

## Description

Create a new IPC endpoint.

## Arguments

Function number (`arg0`) is 9.

The descriptor number to bind to the new IPC endpoint is set in `arg1`.

```
    +----------------------------------------------------------------+
    |                         function = 9                           |  arg0
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

On success, this function returns the descriptor number for the new IPC endpoint
(in `arg0`). On failure, it returns -1 and an error number is set (in `arg1`).

## Errors

* JINUE_EBADF if the specified descriptor is already in use.
* JINUE_EAGAIN if the IPC endpoint could not be created because of insufficient
resources.
