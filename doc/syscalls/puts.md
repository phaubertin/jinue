# PUTS - Write String to Console

## Description

Add a character string to the kernel logs.

## Arguments

Function number (`arg0`) is 3.

The value in `arg1` identifies both the log level (aka. severity) and the
facility. The log level is in the lower byte, i.e. bits 7..0, and the facility
is in bits 15..8 of `arg1`.

The pointer to the string is set in `arg2`. The length of the string, which must
be at most 480 characters, is set in `arg3`.

```
    +----------------------------------------------------------------+
    |                         function = 3                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +------------------------------+-----------------+---------------+
    |          reserved (0)        |    facility     |   log level   |  arg1
    +------------------------------+-----------------+---------------+
    31                              15              8 7              0
    
    +----------------------------------------------------------------+
    |                       address of string                        |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                       length of string                         |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

The string does not need to be null terminated or end with a newline character.

The log level values are aligned with the severity levels described in
[RFC5424 section 6.2.1](https://datatracker.ietf.org/doc/html/rfc5424#section-6.2.1)
and are as follow (include [<jinue/jinue.h>](../../include/jinue/jinue.h) for
the constants definitions):

| Value | Constant                  | Description                              |
|-------|---------------------------|------------------------------------------|
| 0     | JINUE_LOG_LEVEL_EMERGENCY | Emergency: system is unusable            |
| 1     | JINUE_LOG_LEVEL_ALERT     | Alert: action must be taken immediately  |
| 2     | JINUE_LOG_LEVEL_CRITICAL  | Critical: critical conditions            |
| 3     | JINUE_LOG_LEVEL_ERROR     | Error: error conditions                  |
| 4     | JINUE_LOG_LEVEL_WARNING   | Warning: warning conditions              |
| 5     | JINUE_LOG_LEVEL_NOTICE    | Notice: normal but significant condition |
| 6     | JINUE_LOG_LEVEL_INFO      | Informational: informational messages    |
| 7     | JINUE_LOG_LEVEL_DEBUG     | Debug: debug-level messages              |

The facility must be between 1 and 255, with zero being reserved for kernel
use.

## Return Value

On success, this function returns 0 (in `arg0`). On failure, this function
returns -1 and an error number is set (in `arg1`).

## Errors

* JINUE_EINVAL if the specified string length is more than 480.
* JINUE_EINVAL if the specified log level is not recognized.
* JINUE_EINVAL if the specified facility is zero.
