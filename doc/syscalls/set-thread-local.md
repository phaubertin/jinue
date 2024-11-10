# SET_THREAD_LOCAL - Set Thread-Local Storage

## Description

Set the address and size of the thread-local storage for the current thread.

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
