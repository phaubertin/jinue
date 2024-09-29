# CREATE_THREAD - Create a Thread

## Description

Create a new thread in the current process.

## Arguments

Function number (`arg0`) is 4.

The address where code execution will start is set in `arg1`.

The value of the initial stack pointe is set in `arg2`.

```
    +----------------------------------------------------------------+
    |                         function = 4                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                   code entry point address                     |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                      user stack address                        |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         reserved (0)                           |  arg3
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

## Future Direction

Currently, this system call can only create a thread in the current process.
This will be changed so a thread can be created in another process, referred to
by a descriptor.

This system call will also be modified to bind the new thread to a descriptor
so other system calls can refer to it e.g. to destroy it, join it, etc.
