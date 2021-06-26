# Memory Layout

The kernel image file has the following format. It conforms to the Linux boot
protocol version 2.4.

```
  +---------------------------------------+ (file start)-+-
  |              boot sector              |              |
  |            (boot/boot.asm)            |              |
  +---------------------------------------+              | Real mode code
  |           16-bit setup code           |              |
  |            (boot/setup.asm)           |              |
  +---------------------------------------+             -+-                             |
  |           32-bit setup code           |              |                         file |
  |           (boot/setup32.asm)          |              |                       offset |
  +---------------------------------------+              |                    increases |
  |                                       |              |                              |
  |           microkernel (ELF)           |              | Protected mode               V
  |                                       |              | kernel
  +---------------------------------------+              |
  |                                       |              |
  |         process manager (ELF)         |              |
  |                                       |              |
  +---------------------------------------+ (file end)  -+-
```

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
  |         kernel data segment           |                              |                  ^
  |       (copied from ELF binary)        |                              |                  |
  +---------------------------------------+ boot_info.data_physaddr     -+-         address |
  |          kernel stack (boot)          |                              |        increases |
  +-----v---------------------------v-----+ (stack pointer)              |                  |
  |                                       |                              |                  |
  |                . . .                  |                              |
  |                                       |                              | kernel boot
  +-----^---------------------------^-----+ boot_heap                    | stack/heap
  |     kernel heap allocations (boot)    |                              |
  |      kernel physical memory map       |                              |
  +---------------------------------------+ boot_info.boot_heap          |
  |              boot_info                |                              |
  +=======================================+ boot_info.image_top         -+-
  |                                       |                              |
  |         process manager (ELF)         |                              |
  |                                       |                              |
  +---------------------------------------+ boot_info.proc_start         |
  |                                       |                              | kernel image
  |           microkernel (ELF)           |                              |
  |                                       |                              |
  +---------------------------------------+ boot_info.kernel_start       |
  |           32-bit setup code           |                              |
  +---------------------------------------+ boot_info.image_start       -+-
                                            (KLIMIT + 0x100000)
```
  
The boot kernel stack is used only during initialization. Once the first thread
context is created, per-thread kernel stacks are used instead.

Early in the boot process, the 32-bit setup code sets up temporary page tables
with three mappings:
- The first two megabytes of physical memory are identity mapped.
- A few megabytes of memory (BOOT_SIZE_AT_16MB) starting at 0x1000000 (16MB) are
  also identity mapped.
- The second and third megabytes of memory are mapped at 0xc0000000 (KLIMIT).
  This region of physical memory starts with the kernel image as loaded by the
  boot loader. This mapping at KLIMIT is where the kernel expects to run.

```
  +=======================================+ 0x100000000 (4GB)
  |                                       |
  |                                       |
  |               unmapped                |
  |                                       |
  |                                       |
  +=======================================+ 0xc0200000 (KLIMIT + 2MB)
  |                                       |
  |   kernel image + initial allocations  |
  +=======================================+ 0xc0000000 (KLIMIT)
  |                                       |
  |                                       |
  |               unmapped                |
  |                                       |
  |                                       |
  +=======================================+ 0x1c00000 (16MB + BOOT_SIZE_AT_16MB)
  |                                       |
  | physical memory starting at 0x1000000 |
  |                                       |
  +=======================================+ 0x1000000 (16MB)
  |                                       |
  |               unmapped                |
  |                                       |
  +=======================================+ 0x200000 (2MB)
  |   kernel image + initial allocations  |
  +---------------------------------------+ 0x100000 (1MB)
  |  text video memory, boot loader data  |
  +---------------------------------------+ 0
```

The initial page tables set up by the setup code are replaced during the kernel
initialization process. Ultimately, the whole process address space looks like
this:

```
  +---------------------------------------+ 0x100000000 = 4GB
  |                                       |
  |     available for global mappings     |
  |                                       |
  +---------------------------------------+ kernel_vm_top
  |    microkernel, process manager,etc.  |
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
