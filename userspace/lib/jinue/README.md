# The Jinue Library (libjinue)

This library provides the Jinue user space API.

Build artifacts:

* `libjinue.a` provides the interface to the functionality exposed by the
  microkernel. The functions it provides are declared in the `<jinue/jinue.h>`
  C language [header file](../../../include/). This library has no dependency
  whatsoever beyond the kernel.
* `libjinue-utils.a` provides additional utility functions. This library has
  dependencies on the previous one and on a limited number of functions from
  the standard C library (e.g. `vsnprintf()`). It has declarations in the
  following C language header files:

  * `<jinue/logging.h>` contains declarations for logging functions.
  * `<jinue/loader.h>` contains declarations for interfacing with the [user
    space loader](../../loader/).
