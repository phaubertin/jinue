# PUTS - Write String to Console

## Description

Add a character string to the kernel logs.

## Arguments

Function number (`arg0`) is 3.

The value in `arg1` identifies the log level:

* The ASCII character `I` for level "information".
* The ASCII character `W` for level "warning".
* The ASCII character `E` for level "error".

The pointer to the string is set in `arg2`. The length of the string, which must
be at most 120 characters, is set in `arg3`.

The string does not need to be null terminated or end with a newline character.

```
    +----------------------------------------------------------------+
    |                         function = 3                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                           log level                            |  arg1
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                       address of string                        |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                       length of string                         |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

## Return Value

On success, this function returns 0 (in `arg0`). On failure, this function
returns -1 and an error number is set (in `arg1`).

## Errors

* JINUE_EINVAL if the specified string length is more than 480.
* JINUE_EINVAL if the specified log level is not recognized.
