# YIELD_THREAD - Yield From or Destroy Current Thread

## Description

Yield or destroy the current thread. Yielding relinquishes the CPU and allow
other ready threads to run.

## Arguments

Function number (`arg0`) is 5.

If any bit in `arg1` is set (i.e. one), then the current thread is destroyed.
Otherwise, the current thread yields.

```
    +----------------------------------------------------------------+
    |                         function = 5                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +------------------------------------------------------------+---+
    |                         reserved (0)                       | D |  arg1
    +------------------------------------------------------------+---+
    31                                                          2 1  0

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

No return value.

If destroying the current thread, this function does not return.

## Errors

This function always succeeds.

## Future Direction

This system call will be split into two system calls, one that yields and one
that destroys the current thread.

The current system call only affects the current thread. This may be changed to
allow another thread to be destroyed, in which case the thread would be
specified using a descriptor.
