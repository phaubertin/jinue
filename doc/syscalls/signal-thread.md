# SIGNAL_THREAD - Send a Signal to a Thread

## Description

Send a signal to a thread.

For this operation to succeed, either the signal must be sent to a thread in
the current process or the thread descriptor must have the
[JINUE_PERM_SIGNAL](../../include/jinue/shared/asm/permissions.h) permission.

## Arguments

The function number (`arg0`) is 24.

The descriptor that references the thread is passed in `arg1`. Alternatively,
the value -1 may be passed in `arg1` to refer to the current thread.

The signal number is passed in `arg2`. The signal number must be between 1 and
64 inclusive.

```
    +----------------------------------------------------------------+
    |                         function = 24                          |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                   thread descriptor or -1                      |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         signal number                          |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                             flags                              |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

## Return Value

On success, this function returns 0 (in `arg0`). On failure, this function
returns -1 and an error number is set (in `arg1`).
    
## Errors

* JINUE_EINVAL if the signal number is invalid, including zero.
* JINUE_EBADF if the specified descriptor is invalid, or does not refer to a
thread, or is closed.
* JINUE_EPERM if the descriptor does not refer to the current thread and does
not have the signal permission on the thread.
* JINUE_ESRCH if the thread no longer exists.
