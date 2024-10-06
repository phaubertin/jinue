# MMAP - Map Memory

## Description

Map a contiguous block of memory into the address space of a process.

## Arguments

Function number (`arg0`) is 13.

The descriptor number for the target process is set in `arg1`.

A pointer to a [jinue_mmap_args_t structure](../../include/jinue/shared/types.h)
(i.e. the mmap arguments structure) that contains the rest of the arguments is
set in `arg2`.

```
    +----------------------------------------------------------------+
    |                         function = 13                          |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                            process                             |  arg1
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                pointer to mmap arguments structure             |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

The mmap arguments structure contains the following fields:

* `addr` the virtual address (i.e. pointer) of the start of the mapping.
* `length` the length of the mapping, in bytes.
* `prot` the protection flags (see below).
* `paddr` the physical address of the start of the mapped memory.

`addr`, `length` and `paddr` must all be aligned on a page boundary.

`prot` must be set to either `JINUE_PROT_NONE` or to the bitwise or of
`JINUE_PROT_READ`, `JINUE_PROT_WRITE` and/or `JINUE_PROT_EXEC` as described in
the following table:

| Value | Name             | Description           |
|-------|------------------|-----------------------|
| 0     | JINUE_PROT_NONE  | No access             |
| 1     | JINUE_PROT_READ  | Mapping is readable   |
| 2     | JINUE_PROT_WRITE | Mapping is writeable  |
| 4     | JINUE_PROT_EXEC  | Mapping is executable |

If this function fails with a `JINUE_ENOMEM` error, the mapping may have been
partially established.

## Return Value

On success, this function returns 0 (in `arg0`). On failure, this function
returns -1 and an error number is set (in `arg1`).

## Errors

* JINUE_EINVAL if `addr`, `length` and/or `paddr` are not aligned to a page boundary.
* JINUE_EINVAL if any part of the mmap arguments structure as specified by `arg2`
belongs to the kernel.
* JINUE_EINVAL if `prot` is not `JINUE_PROT_NONE` or a bitwise or combination of
`JINUE_PROT_READ`, `JINUE_PROT_WRITE` and/or `JINUE_PROT_EXEC`.
* JINUE_EBADF if the specified descriptor is invalid, or does not refer to a
process, or is closed.
* JINUE_EIO if the process no longer exists.
* JINUE_ENOMEM if not enough memory is available to allocate needed page tables.

## Future Direction

As currently implemented, this system call does not check whether the caller is
authorized to map the specified block memory. This is obviously unacceptable and
will be changed.
