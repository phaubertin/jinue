# Memory Layout

This document describes some of the kernel's initialization steps with their
resulting memory layout.

## Kernel Image Loading

The kernel image file has the following format. It conforms to the
[Linux/x86 Boot Protocol](https://www.kernel.org/doc/html/latest/x86/boot.html)
version 2.04.

```
  +---------------------------------------+ (file start)-+-
  |              boot sector              |              |
  |            (boot/boot.asm)            |              |
  +---------------------------------------+              | Real mode code
  |           16-bit setup code           |              | (16 bits)
  |            (boot/setup.asm)           |              |
  +---------------------------------------+             -+-                             |
  |           32-bit setup code           |              |                         file |
  |           (boot/setup32.asm)          |              |                       offset |
  +---------------------------------------+              |                    increases |
  |                                       |              |                              |
  |           microkernel (ELF)           |              | Protected mode               V
  |                                       |              | kernel
  +---------------------------------------+              | (32 bits)
  |                                       |              |
  |        user space loader (ELF)        |              |
  |                                       |              |
  +---------------------------------------+ (file end)  -+-
```

As described in the
[Linux/x86 Boot Protocol](https://www.kernel.org/doc/html/latest/x86/boot.html),
the boot loader (e.g. [GRUB](https://www.gnu.org/software/grub/)) loads the
16-bit boot sector and setup code at an address somewhere under address 0x100000
(1MB) and the rest of the image starting at address 0x100000 (1MB).

The 16-bit setup code doesn't affect the memory layout in protected mode and
is not used in any way once it is done running and has passed control to the
32-bit setup code. It will not be mentioned further. The 32-bit portion of the
kernel image file loaded starting at address 0x100000 (1 MB) is what we will
refer to as the kernel image.

```
  +---------------------------------------+                        -+-
  |                                       |                         |
  |        user space loader (ELF)        |                         |
  |                                       |                         |
  +---------------------------------------+                         |
  |                                       |                         | kernel image     ^
  |           microkernel (ELF)           |                         |                  |
  |                                       |                         |          address |
  +---------------------------------------+ 0x101000 (1MB + 4kB)    |        increases |
  |           32-bit setup code           |                         |                  |
  +---------------------------------------+ 0x100000 (1MB)         -+-                 |
```

The kernel image is composed of three parts:

* The 32-bit setup code (a single page).
* The microkernel ELF binary. This is the microkernel proper and runs in
kernel mode.
* The user space loader ELF binary. At the end of its initialization, the kernel
loads this binary in user space and gives control to it. This program is
responsible for decompressing the initial RAM disk image, finding the initial
user program, loading it and running it. (TODO the user space loader is not yet
implemented. For now, this binary simply runs some basic functional tests and
prints debugging information.)

## 32-bit Setup Code

The [32-bit setup code](../kernel/interface/i686/setup32.asm) is the first part
of the kernel to run (after the 16-bit setup code if the 16-bit boot protocol
is used). It performs a few tasks necessary to provide the kernel proper the
execution environment it needs. These tasks include:

* Allocating a boot-time stack and heap;
* Copying the BIOS memory map and the kernel command line;
* Loading the kernel's data segment; and
* Allocating and initializing the initial page tables, and then enabling paging.

As it runs, the 32-bit setup code allocates memory pages sequentially right
after the kernel image. Once it completes and passes control to the kernel
binary's entry point, the physical memory starting at address 0x100000 (1MB)
looks like this:

```
  +=======================================+ bootinfo.boot_end           -+-
  |        initial page directory         |                              | 
  +---------------------------------------+ bootinfo.page_directory      |
  |                                       |                              |
  |         initial page tables           | bootinfo.page_table_klimit   |
  |           (PAE disabled)              | bootinfo.page_table_16mb     |
  +---------------------------------------+ bootinfo.page_table_1mb      | setup code
  |         kernel command line           |                              | allocations
  +---------------------------------------+ bootinfo.cmdline             |
  |       BIOS physical memory map        |                              |
  +---------------------------------------+ bootinfo.acpi_addr_map            |    
  |         kernel data segment           |                              |                ^
  |                                       |                              |                |
  +---------------------------------------+ bootinfo.data_physaddr     -+-       address |
  |          kernel stack (boot)          |                              |      increases |
  +-----v---------------------------v-----+ (stack pointer)              |                |
  |                                       |                              |                |
  |                . . .                  |                              |
  |                                       |                              | kernel boot
  +-----^---------------------------^-----+ bootinfo.boot_heap           | stack/heap
  |          kernel heap (boot)           |                              |   
  |              bootinfo                |                               |
  +=======================================+ bootinfo.image_top          -+-
  |                                       |                              |
  |        user space loader (ELF)        |                              |
  |                                       |                              |
  +---------------------------------------+ bootinfo.proc_start          |
  |                                       |                              | kernel image
  |           microkernel (ELF)           |                              |
  |                                       |                              |
  +---------------------------------------+ bootinfo.kernel_start        |
  |           32-bit setup code           |                              |
  +---------------------------------------+ bootinfo.image_start        -+-
                                            0x100000 (1MB)
```

The boot information structure (`bootinfo`) is a data structure allocated on the
boot heap by the 32-bit setup code. It is used to pass information related to
the boot process to the kernel.

The 32-bit setup code does not enable Physical Address Extension (PAE). This is
done later in the initialization process by the kernel itself, if appropriate.

The initial page tables initialized by the 32-bit setup code define three
virtual memory mappings:

1. The first two megabytes of physical memory are identity mapped (virtual
address = physical address). This mapping contains the kernel image, other data
set up by the bootloader as well as the VGA text video memory. The kernel image
is mapped read only while the rest of the memory is mapped read/write.
2. Starting at address 0x1000000 (i.e. 16MB), a few megabytes of memory
(size [BOOT_SIZE_AT_16MB](../include/kernel/interface/i686//asm/boot.h)) are
also identity mapped. Early in the initialization process, the kernel move its
own image there. This region is mapped read/write.
3. The kernel image as well as some of the initial memory allocations, up to but
excluding the initial page tables, are mapped at address 0xc000000 (3GB,
aka. [JINUE_KLIMIT](../include/kernel/infrastructure/i686/asm/shared.h)). This
is the address where the kernel expects to be loaded and ran. This is a linear
mapping with one exception: the kernel's data segment is a copy of the content
in the ELF binary, with appropriate zero-padding for uninitialized data (i.e.
the .bss section) and the copy is mapped read/write at the address where the
kernel expects to find it. The rest of the kernel image is mapped read only and
the rest of the region (initial allocations) is read/write.

```
        +=======================================+ 0x100000000 (4GB)
        |                                       |
        |                                       |
        |               unmapped                |
        |                                       |
        |                                       |
        +=======================================+
    (3) |                                       |
        |   kernel image + initial allocations  |
 +------|                                       |
 |      +=======================================+ 0xc0000000 (JINUE_KLIMIT)
 |      |                                       |
 |      |                                       |
 |      |               unmapped                |
 |      |                                       |
 |      |                                       |
 |      +=======================================+ 0x1c00000 (16MB + BOOT_SIZE_AT_16MB)
 |      |                                       |
 |  (2) | physical memory starting at 0x1000000 |
 |      |                                       |
 |      +=======================================+ 0x1000000 (16MB)
 |      |                                       |
 |      |               unmapped                |
 |      |                                       |
 |      +=======================================+ 0x200000 (2MB)
 |      |                                       |
 |      |   kernel image + initial allocations  |
 +----->|                                       |
    (1) +---------------------------------------+ 0x100000 (1MB)
        | physical memory starting at address 0 |
        |  text video memory, boot loader data  |        
        +---------------------------------------+ 0
```

## Kernel Image Moved and Remapped

One of the first things the kernel does during initialization is to move its
own image to address 0x1000000 (16MB). More specifically, it copies the image
and all initial allocations up to but excluding the initial page tables from
their original location starting at address 0x100000 (1MB) to address 0x1000000
(16MB) and then modifies the virtual memory mapping at JINUE_KLIMIT to map the
copy. This is done in order to free up memory under address 0x1000000 (16MB),
i.e. the ISA DMA limit.

```
        +=======================================+ 0x100000000 (4GB)
        |                                       |
        |                                       |
        |               unmapped                |
        |                                       |
        |                                       |
        +=======================================+
        |                                       |
 +------|   kernel image + initial allocations  |
 |      |                                       |
 |      +=======================================+ 0xc0000000 (JINUE_KLIMIT)
 |      |                                       |
 |      |                                       |
 |      |               unmapped                |
 |      |                                       |
 |      |                                       |
 |      +=======================================+ 0x1c00000 (16MB + BOOT_SIZE_AT_16MB)
 |      |    Memory available for allocation    |
 |      +---------------------------------------+
 |      |                                       |
 +----->|  kernel image + initial allocations   |
        |                                       |
        +=======================================+ 0x1000000 (16MB)
        |                                       |
        |               unmapped                |
        |                                       |
        +=======================================+ 0x200000 (2MB)
        |    Memory available for allocation    |
        +---------------------------------------+ bootinfo.boot_end
        | Initial page tables/page directory    |
        +---------------------------------------+ bootinfo.page_table_1mb
        |               Unused                  |
        |  (moved to address 0x1000000, 16MB)   |
        +---------------------------------------+ 0x100000 (1MB)
        | physical memory starting at address 0 |
        |       (text video memory, etc.)       |        
        +---------------------------------------+ 0
```

The initial page tables and page directory are not moved because they are
temporary. New per-process and global page tables are created later and the
initial page table are then discarded.

## Enabling Physical Address Extension (PAE)

After the kernel image is moved and remapped, PAE is enabled if supported by the
CPU, unless disabled by options on the [kernel command line](cmdline.md). In
order to enable PAE, a new set of page tables and page directories with the
right format must first be allocated and initialized. These new page tables
implement the same mappings as the existing, non-PAE ones (different page table
format, same content).

The new page tables are allocated right after the existing page tables and page
directory, i.e. in the 0-2MB region rather than in the region starting at
address 0x1000000 (16MB).

```
  +---------------------------------------+
  |     initial PAE page directories      |
  +---------------------------------------+
  |                                       |
  |       initial PAE page tables         |
  |                                       |
  +=======================================+ bootinfo.boot_end          -+-
  |        initial page directory         |                              |
  |           (PAE disabled)              |                              |
  +---------------------------------------+ bootinfo.page_directory     |
  |                                       |                              |
  |         initial page tables           | bootinfo.page_table_klimit  | setup code
  |           (PAE disabled)              | bootinfo.page_table_16mb    | allocations
  +---------------------------------------+ bootinfo.page_table_1mb     |
  |                                       |                              |
  |                                       |                              |
  +                                       + bootinfo.data_physaddr     -+-
  |                                       |                              |
  |                                       |                              | original
  |               Unused                  |                              | kernel boot
  |  (moved to address 0x1000000, 16MB)   |                              | stack/heap
  |                                       |                              |
  +                                       + bootinfo.image_top         -+-
  |                                       |                              |
  |                                       |                              | original
  |                                       |                              | kernel image
  |                                       |                              |
  +---------------------------------------+ bootinfo.image_start       -+-
                                            0x100000 (1MB)
```

## Initial Address Space

As part of it initialization, the kernel allocates further memory for its own
use and eventually creates the initial process address space. In this
address space, all the virtual memory under JINUE_KLIMIT belongs to user space.
For this reason, the kernel also remaps video memory in its own region so it
can continue to use it for logging.

Once kernel initialization completes and the kernel passes control to the user
space loader, the memory layout looks like this:

```
  +---------------------------------------+ 0x100000000 = 4GB           -+-
  |                                       |                              |
  |               Unmapped                |                              |
  |                                       |                              |
  |  Available for mapping page frames    |                              |
  |      given back by user space         |                              |
  +=======================================+ JINUE_KLIMIT                 |
  |                                       |   + BOOT_SIZE_AT_16MB        |
  | Available for runtime page allocation |                              | kernel
  |                                       |                              |
  +---------------------------------------+                              |
  |         VGA text video buffer         |                              |
  |            (maps 0xb8000)             |                              |
  +---------------------------------------+                              |
  |                                       |                              |
  |   kernel image + initial allocations  |                              |                ^
  |                                       |                              |                |  
  +=======================================+ JINUE_KLIMIT                -+-       address |
  | (Program arguments, environment, etc.)| (0xc0000000 = 3GB)           |      increases |
  |      User space loader stack          |                              |                |
  +---------------------------------------+                              |                |
  |                                       |                              |
  |               Unmapped                |                              |
  |                                       |                              | user
  +---------------------------------------+                              | space
  |                                       |                              |
  |  User space loader binary code + data |                              |
  |                                       |                              |
  +---------------------------------------+                              |
  |                                       |                              |
  |               Unmapped                |                              |
  |                                       |                              |
  +---------------------------------------+ 0                           -+-
```
