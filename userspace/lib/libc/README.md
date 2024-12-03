# Jinue libc

This library provides a small subset of the standard C and POSIX functions. Its
goal is not be a complete standard C library: an operating system built on top
of the Jinue microkernel is expected to provide its own. Rather, this library
aims:

* To provide the specific functions needed to support the microkernel and the
  [user space loader](../../loader/).
* To serve as a proof of concept for how some of the standard C and POSIX
  functions can be implemented using functionality provided by the microkernel.
  The [test application](../../testapp/) serves as a functional test that calls
  some of these proof of concept functions.

The build artifacts are the following:

* `libc-kernel.a` contains the functions needed by the microkernel. The scope
  of this library is kept to a minimum.
* `libc.a` contains supporting functions for the user space loader and the test
  application. It exclude POSIX thread functions.
* `libpthread.a` contains proof of concept implementations of some POSIX thread
  functions. It is used exclusively by the test application.
