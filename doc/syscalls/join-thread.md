# START_THREAD - Wait for a Thread to Exit

## Description

Wait for a thread to terminate, if it hasn't already, and then retrieve the
pointer-sized value this thread made available when it called
[EXIT_THREAD](exit-thread.md).

## Arguments

Function number (`arg0`) is 21.

The descriptor number for the joined thread is set in `arg1`. This descriptor
must have the necessary permissions (JINUE_PERM_JOIN).

```
    +----------------------------------------------------------------+
    |                         function = 22                          |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                    thread descriptor number                    |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                          reserved (0)                          |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                          reserved (0)                          |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

## Return Value

On success, this function sets `arg0` to 0 and set the value the joined thread
made available when it called [EXIT_THREAD](exit-thread.md) in `arg1`.

On failure, this function sets `arg0` to -1 and an error number in `arg1`.

Note that since this function returns a pointer, it does not follow the usual
convention where `arg0` is used to return a successful value.

## Errors

* JINUE_EBADF if the specified descriptor is invalid, or does not refer to a
thread, or is closed.
* JINUE_EPERM if the target thread descriptor does not have the needed
permissions.
