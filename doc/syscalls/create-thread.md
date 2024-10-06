# CREATE_THREAD - Create a Thread

## Description

Create a new thread in a target process.

## Arguments

Function number (`arg0`) is 4.

The descriptor number for the target process is set in `arg1`.

The address where code execution will start is set in `arg2` and the
value of the initial stack pointer is set in `arg3`.

```
    +----------------------------------------------------------------+
    |                         function = 4                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                            process                             |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                   code entry point address                     |  arg2
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
* JINUE_EAGAIN if the thread could not be created because of needed resources.
* JINUE_EBADF if the specified descriptor is invalid, or does not refer to a
process, or is closed.

## Future Direction

This system call will be modified to bind the new thread to a descriptor so
other system calls can refer to it e.g. to destroy it, join it, etc.
