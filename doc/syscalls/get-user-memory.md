# GET_USER_MEMORY - Get User Memory Map

## Description

This function writes the system memory map in a buffer provided by the caller.

The format is the same as the PC BIOS interrupt 15 hexadecimal, function ax=e820
hexadecimal.

## Arguments

Function number (`arg0`) is 8.

A pointer to the destination buffer is set in `arg2`. The size of the buffer is
set in bits 31..20 of `arg3`.

```
    +----------------------------------------------------------------+
    |                         function = 8                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                         reserved (0)                           |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                        buffer address                          |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +-----------------------+----------------------------------------+
    |     buffer size       |              reserved (0)              |  arg3
    +-----------------------+----------------------------------------+
    31                    20 19                                      0
```

## Return Value

On success, this function returns 0 (in `arg0`). On failure, it returns -1 and
an error number is set (in `arg1`).

## Errors

* JINUE_EINVAL if any part of the destination buffer belongs to the kernel.
* JINUE_EINVAL if the output buffer is larger that 2048 bytes.
* JINUE_E2BIG if the output buffer is too small to fit the result.

## Future Direction

This function provides no information about memory used by the kernel. This will
need to be remediated for this system call to be useful.

The format of the memory map may be changed to one that is architecture
indpendent.

Mapping of the argument may be changed to something more intuitive. (The
rationale behind the current layout is that is is consistent with argument
mapping for the IPC system calls.)

The 2048 bytes restriction on the output buffer size will be eliminated.
