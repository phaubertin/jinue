# Architecture Survey - MMU

## X86-32

Base page size is always 4kB.

With PAE, 2MB pages are supported by referring to them directly from the page
directory. Without PAE, pages mapped in this way are 4MB in size, and a CPUID
check is needed to confirm it is supported.

Bigger pages (e.g. 1GB) are not supported, even with PAE.

PCIDs are not supported.

## AMD64

Base page size is 4kB.

As with PAE, 2MB pages are supported by referring to them directly from the page
directory.

1GB pages are also supported by referring to them directly from a PDPT entry.
CPUID check needed.

Supports 12-bit PCIDs.

## ARMv7-A/32-bit ARMv8-A (ARCH32)

Output address is 40 bits at 4kB granularity with long descriptors. With short
descriptors, its is 32 bits at 4kB granularity or 40 bits at 16MB (supersection)
granularity.

Short descriptors (32 bits, two-level translation):
- Small page: 4kB
- Large page: 64kB
- Section: 1MB
- Supersection: 16MB (optional with exception)

Section or supersection referred to directly from first-level translation table.
Second-level table (aka. page table) is 1kB.

Long descriptors (64 bits, three-level translation):
- Pages: 4kB
- Block: 2MB
- Block: 1GB

For 32-bit input addresses (i.e. stage 1 translation), translation table
structure is very similar to x86 PAE (same 2/9/9/12 address split, descriptor
size, number and size of table at each level). For 40-bit input addresses (i.e.
stage 2), the size of the first-level table is extended to 1024 entries, i.e.
8kB. This 8kB table must be 8kB-aligned.

Either long or short descriptors can be used for PL0&1 stage 1 translation. For
PL2 stage 1 and PL0&1 stage 2 (need virtualization extensions), long descriptors
are mandatory.

## 64-bit ARMv8-A (AARCH64)

Each translation table entry is 64 bits in size.

May support granule size of 4kB, 16kB and/or 64kB. Support of each granule size
is optional. The granule size defines the page size as well as the maximum size
of a translation table.

Implemented physical address size may be 32, 36, 40, 42, 44, 48 or 52 bits.
Virtual address is 48 bits, or 52 bits with large VA support. For both virtual
and physical addresses, extending beyond 48 bits requires the 64kB granule size.

With 4kB granule size, there are four levels of translation tables, all complete
(12 + 4*9 = 48).

Supports address tags in the 8 most significant address bits. Also supports
pointer authentication.

ASID and VMID size are 8 bits or 16 bits (implementation defined).

## RISC-V Draft Privileged Architecture Version 1.10

Three translation modes:  sv32 (32-bit addresses), sv39 (39 bits) and sv48 (48
bits). All three modes use 4kB pages. All translation tables are exactly one
page in size.

sv32 uses 32-bit entries and translate to a 34-bit physical address. It uses
two translation table levels.

sv39 and sv48 both use 64-bit entries and translate to a 56-bit physical
address. sv39 is essentially sv48 with one less table level (3 vs 4).
