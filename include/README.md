# C Language Header Files

* The `jinue/` directory contains the declarations for the Jinue user space
  API. A user space application should only directly include `<jinue/jinue.h>`,
  `<jinue/loader.h>` and/or `<jinue/utils.h>`. See [libjinue](../userspace/lib/jinue/)
  for detail.

  * The `jinue/shared/` directory contains definitions and declarations
    shared between user space and the kernel. The kernel should not include
    files in `jinue/` except for the ones in `jinue/shared/`.

* The `kernel/` directory contains header files intended only for the
  kernel.

* Other files and directories contain standard C and/or POSIX declarations. See
  [libc](../userspace/lib/libc/) for detail.
