# CREATE_IPC - Create IPC Endpoint

## Description

Create a new IPC endpoint.

## Arguments

Function number (`arg0`) is 9.

Flags are set in `arg1` as follow:

* If bit 0 is set, then the created IPC endpoint is a "system" endpoint. (See
Future Direction).
* If bit 1 is set, then, instead if creating a new IPC endpoint, it retrieves
the special-purpose endpoint that allows the process to communicate with its
creating parent. This is useful for service discovery.

```
    +----------------------------------------------------------------+
    |                         function = 9                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                            flags                               |  arg1
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

* JINUE_EAGAIN if a descriptor number could not be allocated.
* JINUE_EAGAIN if the IPC endpoint could not be created because of insufficient
resources.

## Future Direction

This function will be modified so the descriptor number is passed as an argument
by the caller instead of being chosen by the microkernel and returned.

A "system" IPC endpoint is currently no different than a regular IPC endpoint.
The original intent behind the system flag in arguments was to enforce special
access control rules for "system" endpoints. However, the flag will likely be
deprecated instead.
