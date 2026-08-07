# SIGNAL_THREAD - Send a Signal to a Thread

## Description

Send an asynchronous signal to a thread.

For this operation to succeed, the thread descriptor must have the
[JINUE_PERM_SIGNAL](../../include/jinue/shared/asm/permissions.h) permission.

## Arguments

The function number (`arg0`) is 24.

The descriptor that references the thread is passed in `arg1`. Alternatively,
the value -1 may be passed in `arg1` to refer to the current thread.

The signal number is passed in `arg2`. The signal number must be between 1 and
32 inclusive.

A set of flags is passed in `arg3`.

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

`flags` must be set to either `JINUE_SIG_FLAG_NONE` or to `JINUE_SIG_FLAG_SYNC`
as described in the following table:

| Value | Name                  | Description                                       |
|-------|-----------------------|---------------------------------------------------|
| 0     | JINUE_SIG_FLAG_NONE   | Default behaviour                                 |
| 1     | JINUE_SIG_FLAG_SYNC   | Signal is delivered synchronously (if unblocked)  |

The purpose of the `JINUE_SIG_FLAG_SYNC` flag is to support the following POSIX
requirement for the `raise()` function:

> If a signal handler is called, the raise() function shall not return until
> after the signal handler does.

## Return Value

On success, this function returns 0 (in `arg0`). On failure, this function
returns -1 and an error number is set (in `arg1`).
    
## Errors

* JINUE_EINVAL if the signal number is invalid, including zero.
* JINUE_EINVAL if the flags value in `arg3` is invalid, i.e. if unrecognized
flags are set.
* JINUE_EBADF if the specified descriptor is invalid, or does not refer to a
thread, or is closed.
* JINUE_EPERM if the descriptor does not have the signal permission on the
thread.
* JINUE_ESRCH if the thread no longer exists.
