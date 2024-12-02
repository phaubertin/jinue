# User Space Loader

The user space loader is an application that runs in user space whose
executable binary is part of the kernel image. It is given control by the
microkernel as the last step of its boot time initialization. It is responsible
for decompressing and extracting the initial RAM disk image, finding the
initial process' executable binary, loading it and then passing control to it.

The user space loader sets up the [Initial Process Execution Environment](../../doc/init-process.md).
