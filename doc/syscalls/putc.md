# PUTC - Write Character to Console

## Description

Write a single character to the console.

## Arguments

Function number (`arg0`) is 2.

The character is passed in the lower eight bits of arg1.

```
    +----------------------------------------------------------------+
    |                         function = 2                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +------------------------------------------------+---------------+
    |                    reserved (0)                |   character   |  arg1
    +------------------------------------------------+---------------+
    31                                              8 7              0

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

No return value.

## Errors

This function always succeeds.

## Future Direction

A proper logging system call with log levels and a ring buffer in the kernel
will be implemented. Once this happens, this system call will be removed and
replaced.
