# START_THREAD - Start a Thread

## Description

Set the entry point and stack address of a non-running thread specified by a
descriptor number and then start the thread.

For this operation to succeed, the thread descriptor must have the
[JINUE_PERM_START](../../include/jinue/shared/asm/permissions.h) permission.

## Arguments

Function number (`arg0`) is 20.

The descriptor number for the target thread is set in `arg1`.

A pointer to a
[jinue_start_thread_args_t structure](../../include/jinue/shared/types.h) (i.e.
the start thread arguments structure) that contains the rest of the arguments
is set in `arg2`.

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
    |          pointer to start thread arguments structure           |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

The start thread arguments structure contains the following fields:

* `entry` is the address where code execution will start.
* `stack_addr` is the initial stack pointer.
* `sigset` is the initial set of blocked signals for the thread.

## Return Value

On success, this function returns 0 (in `arg0`). On failure, this function
returns -1 and an error number is set (in `arg1`).

## Errors

* JINUE_EINVAL if the pointer to the start thread arguments structure is set to
a kernel address.
* JINUE_EINVAL if the code entry point is set to a kernel address.
* JINUE_EINVAL if the user stack address is set to a kernel address.
* JINUE_EINVAL if the signal set pointer is set to a kernel address.
* JINUE_EBADF if the thread descriptor is invalid, or does not refer to a
thread, or is closed.
* JINUE_EPERM if the thread descriptor does not have the permission to start
the thread.
* JINUE_EBUSY if the thread is already running.
