# The Jinue Microkernel #

**Work in progress**: this is still very much incomplete.

![Screenshot](https://raw.githubusercontent.com/phaubertin/jinue/master/doc/screenshot.png)

Overview of Build Artifacts
---------------------------

The main build artifact that can be built from the code in this
repository is the kernel image, which contains the kernel proper as well
as the user space loader. The kernel image conforms to the
[Linux x86 boot protocol](https://www.kernel.org/doc/Documentation/x86/boot.rst),
which means it can be loaded by a bootloader intended for the Linux
kernel, such as GRUB.

During the boot process, the user space loader expects an initial RAM
disk to have been loaded in memory by the bootloader. This would usually
be provided by the user in order to use the kernel. However, an initial
RAM disk containing a test application can be built from this
repository.

Finally, this repository makes it easy to run the kernel and test
application in QEMU. For this purpose, it builds a bootable ISO image
that contains the kernel image and the initial RAM disk.

Build Requirements
------------------

To build the kernel image and test application, you need a Linux machine
with GNU make and a C compiler that can create 32-bit x86 ELF binaries.
This software is known to build with GCC and clang.

You also need the Netwide Assembler (NASM).

If you wish to build the ISO image for use with QEMU, which is optional,
you need GRUB and GNU xorriso in addition to the above.

Run Time Requirements
---------------------

To run this software, you need either:
* QEMU; or
* A x86 PC on which you have sufficient privileges to configure the boot
manager to load Jinue.

How to Build
------------

This repository imports some dependencies as git submodules. To make
sure these submodules are also cloned and initialized, you need to pass
the `--recurse-submodules` option when you clone the repository.

```
git clone --recurse-submodules https://github.com/phaubertin/jinue.git
```

Alternatively, if you already cloned the repository without passing this
option, you can initialize the submodules separately with the following
command:

```
git submodule update --init
```

Then, configure the source code and its dependencies. You only need to
do this once, not each time you build.

```
./configure
```

To build the ISO image for use with QEMU, build the `qemu` target with
make:

```
make qemu
```

This builds the kernel image and the test application, and then creates
a bootable ISO image.

If you will not be using QEMU and only want to build the kernel image,
this can be done with the default make target:

```
make
```

Similarly, the test application can be built separately with the
`testapp` make target:

```
make testapp
```

How to Run in QEMU
-------------------
You can run the kernel and test application in QEMU using the `qemu-run`
make target:

```
make qemu-run
```

Alternatively, if you don't want QEMU to show a window with the video
output, or if you are on a machine without graphics capabilities, use
the `qemu-run-no-display` target instead. The kernel logs to the serial
port, which QEMU redirects to standard output.

```
make qemu-run-no-display
```

In either case, this generates a log file called `run-jinue.log` in
`devel/qemu/` with the full kernel and test application output.

Finally, you can run the kernel and test application, and then run a
script that performs checks on the log file by using the `qemu-check`
make target:

```
make qemu-check
```

How to Install
--------------

If you will not be using QEMU for testing, you can copy the kernel image
to `/boot` by running the following:

```
sudo make install
```

The full file path of the kernel image installed in this way is
`/boot/jinue`.

Optionally, if you will not be providing your own initial RAM disk file,
you can install the test application initial RAM disk to `/boot` as
well.

```
sudo make install-testapp
```

The full file path of the installed RAM disk file is
`/boot/jinue-testapp-initrd.tar.gz`.

Once this is done, you need to configure your boot loader/manager to
load the installed kernel. Jinue uses the (32-bit or 16-bit) Linux x86
boot protocol, so you can configure your boot manager as if you were
loading a Linux image. This is system dependent but, with GRUB, the
configuration entry might look something like this:

```
menuentry 'Jinue (32-bit boot protocol, test app)' {
    # Replace with the appropriate GRUB module for your filesystem.
	insmod ext2
    # Replace with the partition where you installed Jinue.
	set root='hd0,msdos7'
	set gfxpayload='text'
	echo 'Loading Jinue microkernel...'
    # See command line documentation for details on the command line options.
	linux /boot/jinue vga_enable=yes serial_enable=no DEBUG_DUMP_MEMORY_MAP=1 DEBUG_DUMP_SYSCALL_IMPLEMENTATION=1 DEBUG_DUMP_RAMDISK=1 RUN_TEST_IPC=1
	echo 'Loading test application initial RAM disk...'
	initrd /boot/jinue-testapp-initrd.tar.gz
}
```

For detail on the command line options, see the [command line
documentation](doc/cmdline.md).

For detail on the Linux boot protocol, see
[Documentation/x86/boot.rst](https://www.kernel.org/doc/Documentation/x86/boot.rst)
in the Linux source tree.

Documentation
-------------

There is some documentation in the `doc/` directory]. It is still a work
in progress and some of it is not up to date. The [documentation
README.md file](doc/README.md) links to the parts that are up to date.
