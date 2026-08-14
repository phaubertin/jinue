# GET_SET_SIGNAL_MASK - Get/Set Signal Mask

## Description

Get and/or set the current thread's blocked signals set.

## Arguments

The function number (`arg0`) is 26.

The value in `arg1` indicates how the thread's signal mask is modified, and
thus how the input set argument in `arg2` is interpreted. This argument is
ignored  and treated as if set to `JINUE_SIG_NONE` if `arg2` is null. This
argument must be one of the following values:

| Value | Name              | Description                                                         |
|-------|-------------------|---------------------------------------------------------------------|
| 0     | JINUE_SIG_NONE    | The thread's signal mask is left unmodified.                        |
| 1     | JINUE_SIG_BLOCK   | New signal mask is the union of the current one and `set `.         |
| 2     | JINUE_SIG_SETMASK | New signal mask is set to exactly `set `.                           |
| 3     | JINUE_SIG_UNBLOCK | New signal mask is intersection of current and `set`'s complement.  |

A pointer to the input set, used to modify the thread's blocked signals, is set
in `args2`. If this argument is null, the thread's signal mask is left
unmodified.

A pointer to the output set may be set in `args3`. If non-null, the thread'
blocked signal set before any modification is performed is written there by
this function. It can be left null if getting the current signal set is not
necessary.

```
    +----------------------------------------------------------------+
    |                         function = 26                          |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                              how                               |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                           input set                            |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                           output set                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

## Return Value

On success, this function returns 0 (in `arg0`). On failure, this function
returns -1 and an error number is set (in `arg1`).
    
## Errors

* JINUE_EINVAL if the value of `how` is not one of the allowed values described
above.
* JINUE_EINVAL if `arg2` or `arg3` is set to a kernel address.
