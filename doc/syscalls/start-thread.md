# START_THREAD - Start a Thread

## Description

Set the entry point and stack address on a non-running thread specified by a
descriptor number and then start the thread.

For this operation to succeed, the thread descriptor must have the
[JINUE_PERM_START](../include/jinue/shared/asm/permissions.h) permission.

## Arguments

Function number (`arg0`) is 20.

The descriptor number for the target thread is set in `arg1`.

The address where code execution will start is set in `arg2` and the
value of the initial stack pointer is set in `arg3`.

```
    +----------------------------------------------------------------+
    |                         function = 20                          |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                    thread descriptor number                    |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                    code entry point address                    |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                      user stack address                        |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

## Return Value

On success, this function returns 0 (in `arg0`). On failure, this function
returns -1 and an error number is set (in `arg1`).

## Errors

* JINUE_EINVAL if the code entry point is set to a kernel address.
* JINUE_EINVAL if the user stack address is set to a kernel address.
* JINUE_EBADF if the thread descriptor is invalid, or does not refer to a
thread, or is closed.
* JINUE_EPERM if the thread descriptor does not have the permission to start
the thread.
* JINUE_EBUSY if the thread is already running.

## Future Direction

This system call will be modified to allow a pointer argument to be passed to
the started thread.
