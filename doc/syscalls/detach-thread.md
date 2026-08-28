# DETACH_THREAD - Detach a thread

## Description

Detach a thread. If a thread was [started](start-thread.md) in the joinable
state, it can be detached by calling this function.

For this operation to succeed, the target thread descriptor must have the
[JINUE_PERM_AWAIT](../../include/jinue/shared/asm/permissions.h) permission.

## Arguments

Function number (`arg0`) is 28.

The descriptor number for the target thread is set in `arg1`.

```
    +----------------------------------------------------------------+
    |                         function = 28                          |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                    thread descriptor number                    |  arg1
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

* JINUE_EBADF if the thread descriptor is invalid, or does not refer to a
thread, or is closed.
* JINUE_ESRCH if the thread has not been started or has terminated and has
already been awaited.
* JINUE_EINVAL if the thread is already detached.
* JINUE_EPERM if the thread descriptor does not have the permission to detach
the thread.
