# GET_THREAD_LOCAL - Get Thread Local Storage Address

## Description

Get the address of the thread-local storage for the current thread.

A thread can set the address and size of its thread-local storage by calling the
[SET_THREAD_LOCAL](set-thread-local.md) system call.

## Arguments

Function number (`arg0`) is 7.

```
    +----------------------------------------------------------------+
    |                         function = 7                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                         reserved (0)                           |  arg1
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

This function returns the address of the thread-local storage in `arg0`. If this
has not been set for the current thread by a previous call to the Set
Thread-Local Storage system call, then the return value is zero (i.e. the NULL
pointer in C).

Note that, since this function returns a pointer, it does not follow the usual
convention whereby a negative return value indicates a failure.

## Errors

This function always succeeds.

## Future Direction

Support will be implemented in the kernel for an architecture-dependent
mechanism to access thread-local storage that is faster than calling this
function. On x86, this will likely mean dedicating an entry to thread-local
storage in the GDT. Once this happens, this function may or may not be
deprecated.
