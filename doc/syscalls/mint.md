# DUP - Mint a Descriptor

## Description

Create a descriptor referencing a kernel resource with specified permissions
and cookie.

In order to use this function, the owner descriptor for a kernel resource must
passed as an argument. The owner descriptor for a given kernel resource is the
original descriptor to which the resource was bound when it was created through
the relevant system call (e.g. [CREATE_ENDPOINT](create-endpoint) for an IPC
endpoint). This function binds the same object to a new descriptor with
customized permissions and cookie.

The permissions that can be set on a descriptor depends on the type of kernel
object it refers to. See the
[flag definitions](../../include/jinue/shared/asm/permissions.h) for the
numerical values of the various permission flags.

If the owner descriptor refers to an IPC endpoint, the following permission
flags can be specified:

| Name                  | Description           |
|-----------------------|-----------------------|
| JINUE_PERM_SEND       | Send messages         |
| JINUE_PERM_RECEIVE    | Receive messages      |

If the owner descriptor refers to a process, the following permission flags can
be specified:

| Name                      | Description                               |
|---------------------------|-------------------------------------------|
| JINUE_PERM_CREATE_THREAD  | Create a thread                           |
| JINUE_PERM_MAP            | Map memory into the virtual address space |
| JINUE_PERM_OPEN           | Bind a descriptor                         |

If the owner descriptor refers to a thread, the following permission flags can
be specified:

| Name              | Description                       |
|-------------------|-----------------------------------|
| JINUE_PERM_START  | Start the thread                  |
| JINUE_PERM_JOIN   | Wait for the thread to terminate  |

Other permission flags are reserved.

For this operation to succeed, the descriptor for the target process must have
the [JINUE_PERM_OPEN](../../include/jinue/shared/asm/permissions.h) permission.

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
* `perms` the permission flags of the new descriptor, which must describe a
permissions set appropriate for the type of kernel object the descriptor refers
to. See the [flag definitions](../../include/jinue/shared/asm/permissions.h)
for the numerical values of the permission flags.
* `cookie` the cookie of the new descriptor. This parameter is only meaningful
for references to IPC endpoints and can be set to anything, including zero, for
other resource types.

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
* JINUE_EPERM if the specified owner descriptor is not the owner descriptor of
a kernel resource.
* JINUE_EPERM if the specified process descriptor does not have the permission
to bind a descriptor into the process.

