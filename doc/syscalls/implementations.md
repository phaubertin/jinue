# System Call Implementations

## Overview and Register Mapping

On 32-bit x86 there are three system call implementations: one interrupt-based
implementation and two that use different fast system call instructions.

All three implementations use the same system call register mappings:

* `arg0` maps to CPU register `eax`.
* `arg1` maps to CPU register `ebx`.
* `arg2` maps to CPU register `esi`.
* `arg3` maps to CPU register `edi`.

The microkernel uses auxiliary vector number 8 (called `JINUE_AT_HOWSYSCALL`)
to inform the user space loader and initial process of the implementation it
should use. (See the [Initial Process Execution
Environment](../init-process.md) for detail.)

The value of this auxiliary vector will be set to one of the following:

* 0 for the interrupt-based implementation
* 1 for the SYSCALL/SYSRET (fast AMD) implementation
* 2 for the SYSENTER/SYSEXIT (fast Intel) implementation

## Interrupt-Based Implementation

A system call is invoked by setting the correct arguments in the system call
registers and then generating a software interrupt to interrupt vector 128
(80h).

On return, the return values are set in the system call registers.

## SYSCALL/SYSRET (Fast AMD) Implementation

A system call is invoked by setting the correct arguments in the system call
registers and then executing the `SYSCALL` CPU instruction.

On return, the return values are set in the system call registers.

This implementation is not supported by all CPUs. Only use this system call
implementation if the `JINUE_AT_HOWSYSCALL` auxiliary vector is set to 1.

## SYSENTER/SYSEXIT (Fast Intel) Implementation

A system call is invoked by setting the correct arguments in the system call
registers and then executing the `SYSENTER` CPU instruction.

On return, the return values are set in the system call registers.

This implementation is not supported by all CPUs. Only use this system call
implementation if the `JINUE_AT_HOWSYSCALL` auxiliary vector is set to 2.
