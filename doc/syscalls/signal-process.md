# SIGNAL_PROCESS - Send a Signal to a Process

## Description

Send an asynchronous signal to a process.

For this operation to succeed, the process descriptor must have the
[JINUE_PERM_SIGNAL](../../include/jinue/shared/asm/permissions.h) permission.

## Arguments

The function number (`arg0`) is 23.

The descriptor that references the process is passed in `arg1`. 

The signal number is passed in `arg2`. The signal number must be between 1 and
32 inclusive, or zero. If zero is specified (the null signal), error checking
is performed but no signal is actually sent.

```
    +----------------------------------------------------------------+
    |                         function = 23                          |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                       process descriptor                       |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         signal number                          |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                          reserved (0)                          |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

## Return Value

On success, this function returns 0 (in `arg0`). On failure, this function
returns -1 and an error number is set (in `arg1`).
    
## Errors

* JINUE_EINVAL if the signal number is invalid.
* JINUE_EBADF if the specified descriptor is invalid, or does not refer to a
process, or is closed.
* JINUE_EPERM if the descriptor does not have the signal permission on the
process.
* JINUE_ESRCH if the process no longer exists.
