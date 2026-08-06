# SWAP_SIGNAL_MASK - Swap Signal Mask

## Description

Examine and change a thread's blocked signals.

For this operation to succeed, it must be called by the same or another thread
from the same process.

## Arguments

The function number (`arg0`) is 26.

The descriptor that references the thread is passed in `arg1`. Alternatively,
the value -1 may be passed in `arg1` to refer to the current thread.

A pointer to a
[jinue_swap_signal_mask_args_t structure](../../include/jinue/shared/types.h)
(i.e. the swap signal mask arguments structure) that contains the rest of the
arguments is set in `arg2`.

```
    +----------------------------------------------------------------+
    |                         function = 26                          |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                   thread descriptor or -1                      |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |               swap signal mask arguments structure             |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                          reserved (0)                          |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```
The mint arguments structure contains the following fields:

* `how` indicates how the thread's signal mask is modified, and thus how `set`
is interpreted. Ignored and treated as if set to `JINUE_SIG_NONE` if `set` is
null.
* `set`, if not null, is the signals set used to modify the thread's signal
mask in the way specified by `how`.
* `oset`, if not null, is the address where thread's set of blocked signals
before modification is stored.

The allowed values for `how` and their meaning is as follow:

| Value | Name              | Description                                                         |
|-------|-------------------|---------------------------------------------------------------------|
| 0     | JINUE_SIG_NONE    | The thread's signal mask is left unmodified.                        |
| 1     | JINUE_SIG_BLOCK   | New signal mask is the union the current one and `set `.            |
| 2     | JINUE_SIG_SETMASK | Ner signal mask set to exactly `set `.                              |
| 3     | JINUE_SIG_UNBLOCK | New signal mask is intersection of current and `set`'s complement.  |


## Return Value

On success, this function returns 0 (in `arg0`). On failure, this function
returns -1 and an error number is set (in `arg1`).
    
## Errors

* JINUE_EINVAL if the value of `how` is not one of the allowed values described
above.
* JINUE_EINVAL if `arg2`, `set` and/or `set` is set to a kernel address.
* JINUE_EBADF if the specified descriptor is invalid, or does not refer to a
thread, or is closed.
* JINUE_EPERM if called by a thread from a different process.
* JINUE_ESRCH if the thread no longer exists.
