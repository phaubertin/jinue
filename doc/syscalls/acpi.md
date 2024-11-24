# MMAP - Provide Mapped ACPI Tables

## Description

Provide the mapped ACPI tables to the kernel.

This system call is reserved for use by the user space loader. It fails if
called by another process.

The user space loader must make a best effort to map the relevant ACPI tables
and must call this system call with the result, unless the `JINUE_AT_ACPI_RSDP`
auxiliary vector has been set to zero or omitted.

## Arguments

Function number (`arg0`) is 23.

A pointer to a [jinue_acpi_tables_t structure](../../include/jinue/shared/types.h)
(i.e. the ACPI tables structure) that contains pointers to mapped ACPI tables
is set in `arg1`.

If the user space loader could not map certain ACPI tables, it should set the
relevant pointers to zero (C language `NULL` value) and still call this
function.

```
    +----------------------------------------------------------------+
    |                         function = 23                          |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                 pointer to ACPI tables structure               |  arg1
    +----------------------------------------------------------------+
    31                                                               0
    
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

On success, this function returns 0 (in `arg0`). On failure, this function
returns -1 and an error number is set (in `arg1`).

## Errors

* JINUE_EINVAL if any part of the ACPI tables structure as specified by `arg1`
belongs to the kernel.
* JINUE_ENOSYS if this function does not exist on this CPU architecture.
* JINUE_ENOSYS if this function is called by a process other than the user
space loader.
