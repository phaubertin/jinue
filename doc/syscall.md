# System Call Interface

## Argument Registers

During a system call, information is passed back and forth between user space
and the microkernel through four pointer-sized logical registers called `arg0`
to `arg3`.

When invoking a system call, `arg0` contains the system call number and `arg1`
to `arg3` contain arguments for the call:

* System call numbers 0 to 4095 included are reserved by the microkernel.
* System call numbers 4096 and up all identify the Send Message system call. The
system call number is passed to the recipient as part of the message.

On return from a system call, the contents of arg0 to arg3 depends on the
system call number. Most *but not all* system calls follow the following
convention:

* `arg0` contains a return value, which should be cast as a signed integer (C
type `int`). If the value is positive (including zero), then the call was
successful. A non-zero negative value indicates an error has occurred.
* If the call failed, as indicated by the value in `arg0`, `arg1` contains the
error number. Otherwise, it is set to zero.
* `arg2` and `arg3` are reserved and should be ignored.

## System Call Mechanisms

The microkernel supports the following mechanisms to invoke system calls. Not
all mechanisms are available on all CPUs. On initialization, before invoking any
other system call, an application should call the "Get System Call Mechanism"
system call to determine the best supported system call mechanism. This call of
the "Get System Call Mechanism" system call should be done using the
interrupt-based mechanism, which is the only one garanteed to be supported by
all CPUs.

(TODO should we pass this through the auxiliary vectors instead?)

### Interrupt-Based Mechanism

A system call can be invoked by generating a software interrupt to interrupt
vector 128 (80h). The argument registers are mapped as follow:

* `arg0` maps to CPU register `eax`.
* `arg1` maps to CPU register `ebx`.
* `arg2` maps to CPU register `esi`.
* `arg3` maps to CPU register `edi`.

This system call is always available. However, it is also the slowest, so it is
best to use another one if support is available.

### SYSCALL/SYSRET (Fast AMD) Mechanism

A system call can be invoked by executing the `SYSCALL` CPU instruction.

* `arg0` maps to CPU register `eax`.
* `arg1` maps to CPU register `ebx`.
* `arg2` maps to CPU register `esi`.
* `arg3` maps to CPU register `edi`.

This system call is not supported by all CPUs. It should only be used if the
"Get System Call Mechanism" system call indicates this is the right one to use.

### SYSENTER/SYSEXIT (Fast Intel) Mechanism

A system call can be invoked by executing the `SYSENTER` CPU instruction.

* `arg0` maps to CPU register `eax`.
* `arg1` maps to CPU register `ebx`.
* `arg2` maps to CPU register `esi`.
* `arg3` maps to CPU register `edi`.
* The return address must be set in the `ecx` CPU register.
* The user stack pointer must be set in the `ebp` CPU register.

This system call is not supported by all CPUs. It should only be used if the
"Get System Call Mechanism" system call indicates this is the right one to use.

## System Call Reference

### Get System Call Mechanism

System call number: 1

TODO

### Write Character to Console

System call number: 2

TODO

### Write String to Console

System call number: 3

TODO

### Create a Thread

System call number: 4

TODO

### Yield Current Thread

System call number: 5

TODO

### Set Thread Local Data Address

System call number: 6

TODO

### Get Thread Local Data Address

System call number: 7

TODO

### Get Memory Map

System call number: 8

TODO

### Create IPC Endpoint

System call number: 9

TODO

### Receive Message

System call number: 10

TODO

### Reply to Message

System call number: 11

TODO

### Send Message

System call number: 4096 and up

TODO
