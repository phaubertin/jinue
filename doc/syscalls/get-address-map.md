# GET_ADDRESS_MAP - Get Memory Address Map

## Description

This function writes the system and kernel memory address map to a buffer
provided by the caller.

Specifically, the data written in the buffer is a
[jinue_addr_map_t structure](../../include/jinue/shared/types.h) (the header)
followed by an array of
[jinue_addr_map_entry_t structures](../../include/jinue/shared/types.h) (the map
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
* `type` (32 bits) the type of the block.

The type numbers are described in the table below and are based on the
[ACPI address range types](https://uefi.org/specs/ACPI/6.4_A/15_System_Address_Map_Interfaces.html).

| Type Number | Name                            | Description                                                                                                                                                                                   |
|-------------|---------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1           | `JINUE_MEMYPE_MEMORY`           | Memory available for use.<br><br>Equivalent to ACPI address range type `AddressRangeMemory`.                                                                                                  |
| 2           | `JINUE_MEMYPE_RESERVED`         | Reserved, do not use.<br><br>Equivalent to ACPI address range type `AddressRangeReserved`.                                                                                                    |
| 3           | `JINUE_MEMYPE_ACPI`             | ACPI tables are located in this range. Once the ACPI tables are no longer needed, this range can be used as normal memory.<br><br>Equivalent to ACPI address range type `AddressRangeACPI`.   |
| 4           | `JINUE_MEMYPE_NVS`              | Reserved, do not use. Must be saved and restored across Non-Volatile Sleep (NVS).<br><br>Equivalent to ACPI address range type `AddressRangeNVS`.                                             |
| 5           | `JINUE_MEMYPE_UNUSABLE`         | Reserved, do not use.<br><br>Equivalent to ACPI address range type `AddressRangeUnusable`.                                                                                                    |
| 6           | `JINUE_MEMYPE_DISABLED`         | Reserved, do not use.<br><br>Equivalent to ACPI address range type `AddressRangeDisabled`.                                                                                                    |
| 7           | `JINUE_MEMYPE_PERSISTENT`       | This range is available for use and has byte-addressable non-volatile memory rather that standard RAM.<br><br>Equivalent to ACPI address range type `AddressRangePersistent-Memory`.          |
| 12/0xc      | `JINUE_MEMYPE_OEM`              | Reserved, do not use.<br><br>Equivalent to ACPI "OEM defined" address range type.                                                                                                             |
| 0xf0000000  | `JINUE_MEMYPE_KERNEL_RESERVED`  | Memory reserved for kernel use. This address range cannot be mapped in user space.<br><br>Ranges of this type are page aligned.<br><br>See the Future Direction section below.                |
| 0xf0000001  | `JINUE_MEMYPE_KERNEL_SHARED`    | Shareable memory in use by the kernel. This address range can be mapped read only in user space.<br><br>Ranges of this type are page aligned.<br><br>See the Future Direction section below.  |
| 0xf0000002  | `JINUE_MEMYPE_KERNEL_IMAGE`     | Kernel image.<br><br>This address range can be mapped read only in user space. There will be exactly one entry with this type and a `JINUE_MEMYPE_KERNEL_SHARED` entry with identical range. This range is page aligned. |
| 0xf0000003  | `JINUE_MEMYPE_RAMDISK`          | Compressed RAM disk image.<br><br>This address range can be mapped read only in user space. There will be exactly one entry with this type and a `JINUE_MEMYPE_KERNEL_SHARED` entry with identical range. This range is page aligned. |
| 0xf0000004  | `JINUE_MEMYPE_LOADER_AVAILABLE` | This entry is a hint to the user space loader for memory that it can use for it's own needs. The [Get Memory Information](../init-process.md#get-memory-information-jinue_msg_get_meminfo) message provides a similar functionality for the initial process.<br><br>There will be exactly one entry with this type. This range is page aligned. |
| Any Other   | N/A                             | Reserved for future use. Should be ignored.                                                                                                   |

Notes:

* The system ranges comes directly from the memory map provided by the system
  firmware, without filtering or processing. No assumption should be made
  regarding alignment, overlap, etc.
* A memory manager in user space should use the information reported by this
  function in conjunction with the information provided by the
  [Get Memory Information](../init-process.md#get-memory-information-jinue_msg_get_meminfo)
  message of the [Initial Process Execution Environment](../init-process.md) to
  determine which memory ranges it can allocate to applications.

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

## Future Direction

The memory map reported by this function is static and represents the situation
at the start of the initial user space process. System calls will be added that
will allow a memory manager in user space to give memory to or reclaim memory
from the kernel. The user space memory manager will be responsible for keeping
track of memory ownership changes that it causes. The memory map reported by
this function, specifically the `JINUE_MEMYPE_KERNEL_RESERVED` entries, will not
reflect these changes.
