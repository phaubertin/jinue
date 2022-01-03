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

* The 32-bit setup code.
* The microkernel ELF binary. This is the microkernel proper and runs in
kernel mode.
* The user space loader ELF binary. At the end of its initialization, the kernel
loads this binary in user space and gives control to it. This program is
responsible for decompressing the initial RAM disk image, finding the initial
process, loading it and running it. (TODO the user space loader is not yet
implemented. For now, this binary simply runs some basic functional tests and
prints debugging information. It is still referred to in some places in the
documentation as the "process manager".)

## 32-bit Setup Code

The [32-bit setup code](../boot/setup32.asm) is the first part of the kernel
to run (after the 16-bit setup code if the 16-bit boot protocol is used). It
performs a few tasks necessary to provide the kernel proper the execution
environment it needs. These tasks include:

* Allocating a boot-time stack and heap;
* Copying the BIOS memory map and the kernel command line;
* Loading the kernel's data segment; and
* Allocating and initializing the initial page tables, and then enabling paging.

As it runs, the 32-bit setup code allocates memory right after the kernel image.
Once it completes and passes control to the kernel binary's entry point, the
physical memory starting at address 0x100000 (1MB) looks like this:

```
  +=======================================+ boot_info.boot_end          -+-
  |        initial page directory         |                              | 
  +---------------------------------------+ boot_info.page_directory     |
  |                                       |                              |
  |         initial page tables           | boot_info.page_table_klimit  |
  |           (PAE disabled)              | boot_info.page_table_16mb    |
  +---------------------------------------+ boot_info.page_table_1mb     | setup code
  |         kernel command line           |                              | allocations
  +---------------------------------------+ boot_info.cmdline            |
  |       BIOS physical memory map        |                              |
  +---------------------------------------+ boot_info.e820_map           |    
  |         kernel data segment           |                              |                ^
  |                                       |                              |                |
  +---------------------------------------+ boot_info.data_physaddr     -+-       address |
  |          kernel stack (boot)          |                              |      increases |
  +-----v---------------------------v-----+ (stack pointer)              |                |
  |                                       |                              |                |
  |                . . .                  |                              |
  |                                       |                              | kernel boot
  +-----^---------------------------^-----+ boot_info.boot_heap          | stack/heap
  |          kernel heap (boot)           |                              |   
  |              boot_info                |                              |
  +=======================================+ boot_info.image_top         -+-
  |                                       |                              |
  |        user space loader (ELF)        |                              |
  |                                       |                              |
  +---------------------------------------+ boot_info.proc_start         |
  |                                       |                              | kernel image
  |           microkernel (ELF)           |                              |
  |                                       |                              |
  +---------------------------------------+ boot_info.kernel_start       |
  |           32-bit setup code           |                              |
  +---------------------------------------+ boot_info.image_start       -+-
                                            0x100000 (1MB)
```

The boot information structure (`boot_info`) is a data structure allocated on the
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
(size [BOOT_SIZE_AT_16MB](../include/hal/asm/boot.h)) are also identity mapped.
Early in its initialization process, the kernel move its own image there. This
region is mapped read/write.
3. The kernel image as well as some of the initial memory allocations, up to but
excluding the initial page tables, are mapped at address 0xc000000 (3GB,
aka. [KLIMIT](../include/jinue-common/asm/vm.h)). This is the address where the
kernel expects to be loaded and ran. This is a linear mapping with one
exception: the kernel's data segment is a copy of the content in the ELF binary,
with appropriate zero-padding for uninitialized data (i.e. the .bss section) and
the copy is mapped read/write at the address where the kernel expects to find
it. The rest of the kernel image is mapped read only and the rest of the region
(initial allocations) is read/write.

```
        +=======================================+ 0x100000000 (4GB)
        |                                       |
        |                                       |
        |               unmapped                |
        |                                       |
        |                                       |
        +=======================================+
    (3) |                                       |
 +------|   kernel image + initial allocations  |
 |      +=======================================+ 0xc0000000 (KLIMIT)
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
 +----->|   kernel image + initial allocations  |
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
(16MB) and then modifies the virtual memory mapping at KLIMIT to map the copy.
This is done in order to free up memory under address 0x1000000 (16MB), i.e. the
ISA DMA limit.

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
 |      +=======================================+ 0xc0000000 (KLIMIT)
 |      |                                       |
 |      |                                       |
 |      |               unmapped                |
 |      |                                       |
 |      |                                       |
 |      +=======================================+ 0x1c00000 (16MB + BOOT_SIZE_AT_16MB)
 |      |                                       |
 |      | physical memory starting at 0x1000000 |
 +----->|                                       |
        +=======================================+ 0x1000000 (16MB)
        |                                       |
        |               unmapped                |
        |                                       |
        +=======================================+ 0x200000 (2MB)
        |                                       |
        |   kernel image + initial allocations  |
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
CPU unless disabled by options on the kernel command line. (TODO support for
the kernel command line is not yet implemented.) In order to enable PAE, a new
set of page tables and page directories with the right format must first be
allocated and initialized. These new page tables implement the same mappings
as the existing, non-PAE ones.

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
  +=======================================+ boot_info.boot_end          -+-
  |        initial page directory         |                              |
  |           (PAE disabled)              |                              |
  +---------------------------------------+ boot_info.page_directory     |
  |                                       |                              |
  |         initial page tables           | boot_info.page_table_klimit  | setup code
  |           (PAE disabled)              | boot_info.page_table_16mb    | allocations
  +---------------------------------------+ boot_info.page_table_1mb     |
  |                                       |                              |
  |                                       |                              |
  +                                       + boot_info.data_physaddr     -+-
  |                                       |                              |
  |                                       |                              | original
  |               Unused                  |                              | kernel boot
  |  (moved to address 0x1000000, 16MB)   |                              | stack/heap
  |                                       |                              |
  +                                       + boot_info.image_top         -+-
  |                                       |                              |
  |                                       |                              | original
  |                                       |                              | kernel image
  |                                       |                              |
  +---------------------------------------+ boot_info.image_start       -+-
                                            0x100000 (1MB)
```

## TODO

Once it is loaded in memory by the boot loader and all the setup, startup and
initialization code has run, the layout in memory is as shown below. boot_info
is a data structure allocated on the boot heap by the 32-bit setup code.

```
  +---------------------------------------+
  |   more initial allocations by kernel  |
  |      (1:1 virtual memory mapping)     |
  +---------------------------------------+
  |          VGA text video buffer        |
  |             (maps 0xb8000)            |
  +---------------------------------------+
  |   initial page allocations by kernel  |
  |      (1:1 virtual memory mapping)     |
  +=======================================+ boot_info.boot_end          -+-
  |        initial page directory         |                              | 
  +---------------------------------------+ boot_info.page_directory     |
  |         initial page tables           |                              |
  |           (PAE disabled)              |                              |
  +---------------------------------------+ boot_info.page_table         | setup code
  |         kernel command line           |                              | allocations
  +---------------------------------------+ boot_info.cmdline            |
  |       BIOS physical memory map        |                              |
  +---------------------------------------+ boot_info.e820_map           |    
  |         kernel data segment           |                              |                ^
  |       (copied from ELF binary)        |                              |                |
  +---------------------------------------+ boot_info.data_physaddr     -+-       address |
  |          kernel stack (boot)          |                              |      increases |
  +-----v---------------------------v-----+ (stack pointer)              |                |
  |                                       |                              |                |
  |                . . .                  |                              |
  |                                       |                              | kernel boot
  +-----^---------------------------^-----+ boot_heap                    | stack/heap
  |     kernel heap allocations (boot)    |                              |
  |      kernel physical memory map       |                              |
  +---------------------------------------+ boot_info.boot_heap          |
  |              boot_info                |                              |
  +=======================================+ boot_info.image_top         -+-
  |                                       |                              |
  |        user space loader (ELF)        |                              |
  |                                       |                              |
  +---------------------------------------+ boot_info.proc_start         |
  |                                       |                              | kernel image
  |           microkernel (ELF)           |                              |
  |                                       |                              |
  +---------------------------------------+ boot_info.kernel_start       |
  |           32-bit setup code           |                              |
  +---------------------------------------+ boot_info.image_start       -+-
```
  
The boot kernel stack is used only during initialization. Once the first thread
context is created, per-thread kernel stacks are used instead.


----

The initial page tables set up by the setup code are replaced during the kernel
initialization process. Ultimately, the whole process address space looks like
this:

```
  +---------------------------------------+ 0x100000000 = 4GB
  |                                       |
  |     available for global mappings     |
  |                                       |
  +---------------------------------------+ kernel_vm_top
  | microkernel, user space loader, etc.  |
  |              (see above)              |  
  +=======================================+ KLIMIT (0xc0000000 = 3GB)
  |                                       |
  |                                       |
  |          User memory space            |
  |                                       |
  |                                       |
  |                                       |
  +---------------------------------------+ 0
```

All memory mappings from 0 to KLIMIT are user mappings which belong to the
currently running process. Mappings above KLIMIT are global kernel mappings.
