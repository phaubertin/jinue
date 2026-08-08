# SET_SIGNAL_HANDLER - Set Signal Handler

## Description

Set the current process' signal handling function.

## Arguments

Function number (`arg0`) is 27.

The address of the signal handling function is set in `arg1`.

```
    +----------------------------------------------------------------+
    |                         function = 27                          |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                    signal handler address                      |  arg1
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

On success, this function returns 0 (in `arg0`). On failure, it returns -1 and
an error number is set (in `arg1`).

## Errors

* JINUE_EINVAL if the signal handler address set with this function belongs to
the kernel.
