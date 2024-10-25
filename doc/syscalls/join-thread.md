# JOIN_THREAD - Wait for a Thread to Exit

## Description

Wait for a thread to terminate, if it hasn't already. The target thread must
have been started with [START_THREAD](start-thread.md) and must not be the
calling thread. Furthermore, this function must be called at most once on a
given thread since it has been started.

## Arguments

Function number (`arg0`) is 21.

The descriptor number for the joined thread is set in `arg1`. This descriptor
must have the necessary permissions (JINUE_PERM_JOIN).

```
    +----------------------------------------------------------------+
    |                         function = 21                          |  arg0
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

On success, this function returns 0 (in `arg0`). On failure, this function
returns -1 and an error number is set (in `arg1`).

## Errors

* JINUE_EBADF if the specified descriptor is invalid, or does not refer to a
thread, or is closed.
* JINUE_EPERM if the target thread descriptor does not have the needed
permissions.
* JINUE_ESRCH if the thread has not been started or has already beed joined.
* JINUE_EDEADLK if a thread attempts to join itself.
