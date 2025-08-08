# Memory Layout

This document describes some of the kernel's initialization steps with their
resulting memory layout.

## Kernel Image Loading

The kernel image file has the following format. It conforms to the
[Linux/x86 Boot Protocol](https://www.kernel.org/doc/html/latest/x86/boot.html)
version 2.04.

```
  +---------------------------------------+ (file start)---
  |                                       |              ^
  |   16-bit boot sector and setup code   |              | Real mode code
  |            (setup16.asm)              |              v (16 bits)
  +---------------------------------------+             ---                             |
  |          32-bit setup code            |              ^                         file |
  |            (setup32.asm)              |              |                       offset |
  +---------------------------------------+              |                    increases |
  |                                       |              |                              |
  |           microkernel (ELF)           |              | Protected mode               V
  |                                       |              | kernel
  +---------------------------------------+              | (32 bits)
  |                                       |              |
  |        user space loader (ELF)        |              |
  |                                       |              v
  +---------------------------------------+ (file end)  ---
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
  +---------------------------------------+                        ---
  |                                       |                         ^
  |        user space loader (ELF)        |                         |
  |                                       |                         |
  +---------------------------------------+                         |
  |                                       |                         | kernel image     ^
  |           microkernel (ELF)           |                         |                  |
  |                                       |                         |          address |
  +---------------------------------------+ 0x101000 (1MB + 4kB)    |        increases |
  |           32-bit setup code           |                         v                  |
  +---------------------------------------+ 0x100000 (1MB)         ---                 |
```

The kernel image is composed of three parts:

* The 32-bit setup code (a single page).
* The microkernel ELF binary. This is the microkernel proper and runs in
kernel mode.
* The user space loader ELF binary. At the end of its initialization, the kernel
loads this binary in user space and gives control to it. This program is
responsible for decompressing the initial RAM disk image, finding the initial
user program, loading it and running it.

## 32-bit Setup Code

The [32-bit setup code](../kernel/interface/i686/setup/setup32.asm) is the
first part of the kernel to run (after the 16-bit setup code if the 16-bit boot
protocol is used). It performs a few tasks necessary to provide the kernel
proper the execution environment it needs. These tasks include:

* Allocating and setting up a boot-time stack and heap;
* Copying the BIOS memory map and the kernel command line;
* Loading the kernel's data segment; and
* Allocating and initializing the kernel page tables, then enabling paging.

As it runs, the 32-bit setup code allocates memory pages sequentially starting
at 0x1000000 (16MB). Once it completes, it passes control to the kernel ELF
binary entry point. When it does so, it passes passes information to the kernel
using the boot information structure (aka. bootinfo). This structure is
declared as type [bootinfo_t](kernel/interface/i686/types.h).

When the 32-bit setup code completes, physical memory looks like this:

```
  +===================================+                         ===
  |      initial page directory       |                          ^
  +-----------------------------------+ bootinfo.cr3             |
  |       initial page tables         |     (if PAE disabled)    |
  +-----------------------------------+                          | setup code
  |       kernel command line         |                          | page
  +-----------------------------------+                          | allocations
  |     BIOS physical memory map      |                          |
  +-----------------------------------+                          |
  |       kernel data segment         |                          |
  |(copied from ELF binary and padded)|                          v
  +-----------------------------------+                         ---
  |        kernel stack (boot)        |                          ^
  +-----v-----------------------v-----+                          |
  |                                   |                          |
  |              . . .                |                          | kernel boot
  |                                   |                          | stack/heap
  +-----^-----------------------^-----+                          |
  |      PDPT (if PAE enabled)        |                          |
  +-----------------------------------+ bootinfo.cr3             |
  |            bootinfo               |     (if PAE enabled)     |
  |                                   |                          v
  +===================================+ 0x1000000 (16MB)        ===                ^
  |                                   |                                            |
  .                                   .                                            |
  .             Unused                .                                    address |
  .                                   .                                  increases |
  |                                   |                                            |
  +===================================+                         ===                |
  |                                   |                          ^
  |      user space loader (ELF)      |                          |
  |                                   |                          |
  +-----------------------------------+                          |
  |                                   |                          | kernel image
  |         microkernel (ELF)         |                          |
  |                                   |                          |
  +-----------------------------------+                          |
  |         32-bit setup code         |                          |
  |         (i.e. this code)          |                          v
  +===================================+ 0x100000 (1MB)          ===
```

This gets mapped into the kernel address space with the following layout:

```
  +===================================+ bootinfo.boot_end       ===
  |      initial page directory       |                          ^
  +-----------------------------------+ bootinfo.page_directory  |
  |       initial page tables         |                          |
  +-----------------------------------+ bootinfo.page_table      | setup code
  |       kernel command line         |                          | page
  +-----------------------------------+ bootinfo.cmdline         | allocations
  |     BIOS physical memory map      |                          |
  +-----------------------------------+ bootinfo.acpi_addr_map   |
  |    kernel data segment (alias)    |                          v
  +-----------------------------------+                         ---
  |        kernel stack (boot)        |                          ^
  +-----v-----------------------v-----+ (stack pointer)          |
  |                                   |                          |
  |              . . .                |                          | kernel boot
  |                                   |                          | stack/heap
  +-----^-----------------------^-----+ bootinfo.boot_heap       |
  |      PDPT (if PAE enabled)        |                          |
  +-----------------------------------+                          |
  |            bootinfo               |                          v
  +===================================+ bootinfo                ===                ^
  |                                   |     = ALLOC_BASE                           |
  .                                   .                                            |
  .             Unused                .                                    address |
  .                                   .                                  increases |
  |                                   |                                            |
  +===================================+                         ===                |
  |                                   |                          ^
  |      user space loader (ELF)      |                          |
  |                                   |                          |
  +-----------------------------------+ bootinfo.loader_start    |
  |                                   |                          | kernel image
  |            microkernel            |                          |
  |      code and data segments       |                          |
  |                                   |                          |
  +-----------------------------------+ bootinfo.kernel_start    |
  |         32-bit setup code         |                          |
  |         (i.e. this code)          |                          v
  +===================================+ bootinfo.image_start    ===
                                            = KERNEL_BASE
                                            = JINUE_KLIMIT (0xc0000000 = 3GB)
```
