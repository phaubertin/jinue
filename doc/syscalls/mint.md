# DUP - Mint a Descriptor

## Description

Create a descriptor referencing a kernel resource with specified cookie and
permissions.

In order to use this function, the owner descriptor for the resource must be
specified. The owner descriptor is the descriptor that was specified in the
call to the function that created the resource (e.g. CREATE_ENDPOINT).

## Arguments

Function number (`arg0`) is 19.

The number of the owner descriptor number is set in `arg1`.

A pointer to a [jinue_mint_args_t structure](../../include/jinue/shared/types.h)
(i.e. the mint arguments structure) that contains the rest of the arguments is
set in `arg2`.

```
    +----------------------------------------------------------------+
    |                        function = 19                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                       owner descriptor                         |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                pointer to mint arguments structure             |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                          reserved (0)                          |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

The mint arguments structure contains the following fields:

* `process` a descriptor to the target process in which to create the new
descriptor.
* `fd` descriptor number in the target process in which to place the new
descriptor.
* `perms` the permission flags of the new descriptor (see below).
* `cookie` the cookie of the new descriptor. This parameter is only meaningful
for references to IPC endpoints and can be set to anything, including zero, for
other resource types.

`perms` must be set to the bitwise or of permission flags that are appropriate
for the type of kernel object to which the owner descriptor refers.

If the owner descriptor refers to an IPC endpoint, the following permission
flags can be specified:

| Value | Name                  | Description                       |
|-------|-----------------------|-----------------------------------|
| 1     | JINUE_PERM_SEND       | Can be used to send messages      |
| 2     | JINUE_PERM_RECEIVE    | Can be used to receive messages   |

Other permission flags are reserved.

## Return Value

On success, this function returns 0 (in `arg0`). On failure, this function
returns -1 and an error number is set (in `arg1`).

## Errors

* JINUE_EINVAL if any bits are set in `perms` other than permission flags
appropriate for the type of kernel object to which the owner descriptor refers.
* JINUE_EBADF if the specified owner descriptor is invalid or is closed.
* JINUE_EBADF if the specified process descriptor is invalid, does not refer
to a process or is closed.
* JINUE_EBADF if the descriptor specified in the `fd` argument is invalid or is
already in use.
* JINUE_EPERM if the specified descriptor is not the owner descriptor.
