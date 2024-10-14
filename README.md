# The Jinue Microkernel #

**Work in progress**: this is still very much incomplete.

![Screenshot](https://raw.githubusercontent.com/phaubertin/jinue/master/doc/screenshot.png)

Build Requirements
------------------

To build the Jinue microkernel, you will need a Linux machine with GNU make and
a C compiler that can create 32-bit x86 binaries. This software is known to
build with GCC and clang.

You will also need the Netwide Assembler (NASM).

If you wish to build an ISO image for use with QEMU, which is optional,
you will need GRUB and GNU xorriso.

Runtime Requirements
--------------------

To boot the Jinue microkernel, you will need either:
* QEMU; or
* A x86 PC on which you have sufficient privileges to configure the boot manager
to load Jinue.

How to Build
------------

This repository imports some dependencies as git submodules. To make sure these submodules are also cloned and initialized, make sure you pass the `--recurse-submodules` option when you clone the repository.

```
git clone --recurse-submodules https://github.com/phaubertin/jinue.git
```
Alternatively, if you already cloned the repository without passing this option, you can clone and initialize the submodules separately with the following command:

```
git submodule update --init
```

Then, configure the source code and its dependencies. You only need to do this once and not for each build.
```
./configure
```

To build the ISO image for use with QEMU, build the `qemu` target with make:
```
make qemu
```
If you will not be using QEMU and only want to build the kernel image, this can be done with the default make target:
```
make
```

How to Run in QEMU
-------------------
You can run the kernel and test application in QEMU using the `qemu-run` make target:
```
make qemu-run
```
Alternatively, if you don't want QEMU to show a window with the video output, or if you are on a machine without graphics capabilities, use the `qemu-run-no-display` target instead. The kernel logs to the serial port, which QEMU redirects to standard output.
```
make qemu-run-no-display
```

How to Run in VirtualBox (Unmaintained)
------------------------
First build the vbox target as indicated above:
```
make vbox
```
This builds the `bin/jinue.iso` ISO file inside the source tree.

Now, in VirtualBox, create a new virtual machine called `Jinue`. Use the Other
machine type and Other/Unknown in the version field. Do not specify a virtual
hard disk. Then, open the configuration for your new virtual machine, choose
Storage in the side bar. In the IDE controller section, add a CDROM drive if
there isn't already one and specify the `bin/jinue.iso` ISO file inside the
source tree as the file for this CDROM drive.

Optionally, you can also configure the virtual machine to redirect the first
COM port (COM1) to a file. The kernel logs messages to COM1.

Once the virtual machine has been created, you can re-build the ISO image and
then start the virtual machine by using the `vbox-run` make target:
```
make vbox-run
```
The `vbox-debug` make target does the same except it enables the VirtualBox
debugger:
```
make vbox-debug
```

How to Install
--------------

If you will not be using VirtualBox, you can copy the kernel image to `/boot` by
running the following:
```
sudo make install
```
The copied kernel image file is `/boot/jinue`.

Once this is done, you need to configure your boot loader/manager to load this
kernel. Jinue uses the 16-bit Linux boot protocol, so you can configure your
boot manager as if you were loading a Linux image with the 16-bit boot protocol
([linux16 command](devel/virtualbox/grub.cfg#L29) if using GRUB).

For detail on the Linux boot protocol, see
[documentation/x86/boot.txt](https://www.kernel.org/doc/Documentation/x86/boot.txt)
in the Linux source tree.

Documentation
-------------

There is some documentation in the [doc/ directory](doc/README.md). It is still
very much a work in progress.
