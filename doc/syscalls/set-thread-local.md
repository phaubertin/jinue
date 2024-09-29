# SET_THREAD_LOCAL - Set Thread-Local Storage

## Description

Set the address and size of the thread-local storage for the current thread.

A thread can then retrieve the address of the thead-local storage by calling the
[GET_THREAD_LOCAL](get-thread-local.md) system call.

## Arguments

Function number (`arg0`) is 6.

The address of (i.e. pointer to) the thread-local storage is set in `arg1`.

The size, in bytes, of the thread-local storage is set in `arg2`.

```
    +----------------------------------------------------------------+
    |                         function = 6                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                 thread-local storage address                   |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                   thread-local storage size                    |  arg2
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

* JINUE_EINVAL if any part of the region set with this function belongs to the
kernel.

## Future Direction

The current system call can only set the thread-local storage for the current
thread. This may be changed to allow setting the thread-local storage for
another thread, which would be specified using a descriptor.

Support will also be implemented in the kernel for an architecture-dependent
mechanism to access thread-local storage that is faster than calling the
[GET_THREAD_LOCAL](get-thread-local.md) system call. On x86, this will likely
mean dedicating an entry to thread-local storage in the GDT.
