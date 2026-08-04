# SWAP_SIGNAL_MASK - Swap Signal Mask

## Description

Examine and change a thread's blocked signals.

For this operation to succeed, it must be called by the same or another thread
from the same process.

## Arguments

The function number (`arg0`) is 26.

The descriptor that references the thread is passed in `arg1`. Alternatively,
the value -1 may be passed in `arg1` to refer to the current thread.

The signal mask is passed in `arg2`. The value passed in `arg3` determines how
this value is interpreted:

| Value | Name              | Description                                                        |
|-------|-------------------|--------------------------------------------------------------------|
| 0     | JINUE_SIG_NONE    | The thread's signal mask is left unmodified                        |
| 1     | JINUE_SIG_BLOCK   | New signal mask is the union the current one and `arg2`            |
| 2     | JINUE_SIG_SETMASK | Ner signal mask set to exactly `arg2`                              |
| 3     | JINUE_SIG_UNBLOCK | New signal mask is intersection of current and `arg2`'s complement |


```
    +----------------------------------------------------------------+
    |                         function = 26                          |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                       thread descriptor                        |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                          signal mask                           |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                              how                               |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

## Return Value

On success, this function returns 0 (in `arg0`) and set the thread's signal
mask before any change was performed in `arg1`.

On failure, this function returns -1 and an error number is set (in `arg1`).
    
## Errors

* JINUE_EINVAL if the value in `arg2` is not one of the values described above.
* JINUE_EBADF if the specified descriptor is invalid, or does not refer to a
thread, or is closed.
* JINUE_EPERM if called by a thread from a different process.
* JINUE_ESRCH if the thread no longer exists.
