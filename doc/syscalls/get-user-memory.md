# GET_USER_MEMORY - Get User Memory Map

## Description

This function writes the system memory map to a buffer provided by the caller.

Specifically, the data written in the buffer is a
[jinue_mem_map_t structure](../../include/jinue/shared/types.h) (the header)
followed by an array of
[jinue_mem_entry_t structures](../../include/jinue/shared/types.h) (the map
entries).

Each map entry describes a contiguous block of memory. Some entries describe
memory regions reported by the system firmware whereas other entries describe
memory regions of interest reported by the kernel itself.

The header contains a single 32-bit field (`num_entries`) that specifies the
number of map entries. If the call fails with `JINUE_E2BIG`, which indicates the
buffer it too small to receive the full memory map, the kernel still sets this
field to the total number of map entries, as long as the buffer is at least
large enough for the header.

Each map entry contains the following fields, in this order:

* `addr` (64 bits) the address of the start of the memory block.
* `size` (64 bits) the size of the block, in bytes.
* `type` (32 bits) the type of the block, as described in the table below.

| Type Number | Name                            | Description                                                                                                                                                                                                                                                                                             |
|-------------|---------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 0           | JINUE_MEM_TYPE_BIOS_RESERVED    | Memory reported as unusable by the system firmware.<br><br>This range comes directly from the memory map provided by the system firmware, without filtering or processing. No assumption should be made regarding alignment, overlap, etc.                                                              |
| 1           | JINUE_MEM_TYPE_AVAILABLE        | Memory reported as available for use by the system firmware.<br><br>This range comes directly from the memory map provided by the system firmware, without filtering or processing. No assumption should be made regarding alignment, overlap, etc.                                                     |
| 2           | JINUE_MEM_TYPE_ACPI             | Memory reported by the system firmware as used for ACPI tables and reclaimable once these tables have been read.<br><br>This range comes directly from the memory map provided by the system firmware, without filtering or processing. No assumption should be made regarding alignment, overlap, etc. |
| 3           | JINUE_MEM_TYPE_RAMDISK          | Compressed RAM disk image.<br><br>This region can be mapped read only. There will be exactly one entry with this type.                                                                                                                                                                                  |
| 4           | JINUE_MEM_TYPE_KERNEL_IMAGE     | Kernel image.<br><br>This region can be mapped read only. There will be exactly one entry with this type.                                                                                                                                                                                               |
| 5           | JINUE_MEM_TYPE_KERNEL_RESERVED  | Memory reserved for kernel use.<br><br>Cannot be mapped. Entries of this type reflect the situation at the end of kernel initialization. They can no longer be relied upon once the memory manager in user space has started giving memory to or reclaiming memory from the kernel.                     |
| 6           | JINUE_MEM_TYPE_LOADER_AVAILABLE | This entry is a hint to the user space loader for memory that it can use for it's own needs.<br><br>There will be exactly one entry with this type.                                                                                                                                                     |

## Arguments

Function number (`arg0`) is 8.

A pointer to the destination buffer is set in `arg1`. The size of the buffer is
set in `arg2`.

```
    +----------------------------------------------------------------+
    |                         function = 8                           |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                        buffer address                          |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         buffer size                            |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                         reserved (0)                           |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

## Return Value

On success, this function returns 0 (in `arg0`). On failure, it returns -1 and
an error number is set (in `arg1`).

## Errors

* JINUE_EINVAL if any part of the destination buffer belongs to the kernel.
* JINUE_E2BIG if the output buffer is too small to fit the result.
