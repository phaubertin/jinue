# EXIT_THREAD - Exit the Current Thread

## Description

Terminate the current thread and make available a pointer-sized value to any
thread that successfully joins the terminating thread by calling
[JOIN_THREAD](join-thread.md).

## Arguments

Function number (`arg0`) is 12.

The value made available to the joining thread is set in `arg1`.

```
    +----------------------------------------------------------------+
    |                        function = 12                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                         exit value                             |  arg1
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

This function terminates the current thread and does not return.

## Errors

This function always succeeds.
