# MCLONE - Clone a Memory Mapping

## Description

Clone a contiguous memory mapping from one process to another.

For this operation to succeed, the destination process descriptor must have the
[JINUE_PERM_MAP](../../include/jinue/shared/asm/permissions.h) permission.

## Arguments

Function number (`arg0`) is 15.

The descriptor number for the source process is set in `arg1`.

The descriptor number for the destination process is set in `arg2`.

A pointer to a [jinue_mclone_args_t structure](../../include/jinue/shared/types.h)
(i.e. the mclone arguments structure) that contains the rest of the arguments is
set in `arg3`.

```
    +----------------------------------------------------------------+
    |                         function = 15                          |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                             src                                |  arg1
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                             dest                               |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |               pointer to mclone arguments structure            |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

The mclone arguments structure contains the following fields:

* `src_addr` the virtual address (i.e. pointer) of the start of the mapping in
the source address space.
* `dest_addr` the virtual address (i.e. pointer) of the start of the mapping in
the destination address space.
* `length` the length of the mapping, in bytes.
* `prot` the protection flags (see below).

`src_addr`, `dest_addr` and `length` must all be aligned on a page boundary.

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
partially cloned.

## Return Value

On success, this function returns 0 (in `arg0`). On failure, this function
returns -1 and an error number is set (in `arg1`).

## Errors

* JINUE_EINVAL if `src_addr`, `dest_addr` and/or `length` are not aligned to a
page boundary.
* JINUE_EINVAL if any part of the mclone arguments structure as specified by `arg3`
belongs to the kernel.
* JINUE_EINVAL if `prot` is not `JINUE_PROT_NONE` or a bitwise or combination of
`JINUE_PROT_READ`, `JINUE_PROT_WRITE` and/or `JINUE_PROT_EXEC`.
* JINUE_EBADF if one of the specified descriptors is invalid, or does not refer
to a process, or is closed.
* JINUE_EIO if either process no longer exists.
* JINUE_ENOMEM if not enough memory is available to allocate needed page tables.
* JINUE_EPERM if the destination process descriptor does not have the
permission to map memory into the process.
