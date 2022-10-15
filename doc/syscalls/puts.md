# PUTS - Write String to Console

## Description

Write a character string to the console.

## Arguments

Function number (`arg0`) is 3.

The pointer to the string is set in `arg1`.

The length of the string is passed in bits 19..8 of `arg3`. The string does not
have to be NUL-terminated.

```
    +----------------------------------------------------------------+
    |                         function = 3                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                         reserved (0)                           |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                      address of string                         |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +-----------------------+------------------------+---------------+
    |     reserved (0)      |         length         |  reserved (0) |  arg3
    +-----------------------+------------------------+---------------+
    31                    20 19                     8 7              0
```

## Return Value

No return value.

## Errors

This function always succeeds.

## Future Direction

A proper logging system call with log levels and a ring buffer in the kernel
will be implemented. Once this happens, this system call will be removed and/or
replaced.
