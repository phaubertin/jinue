# System Call Implementations

The microkernel supports the following implementations to invoke system calls.
Not all implementations are available on all CPUs. On initialization, before
invoking any system call, an application should look at the value of auxiliary
vector number 8 (called `JINUE_AT_HOWSYSCALL`) to determine the best supported
system call implementation:

* 0 for the interrupt-based implementation
* 1 for the SYSCALL/SYSRET (fast AMD) implementation
* 2 for the SYSENTER/SYSEXIT (fast Intel) implementation

## Interrupt-Based Implementation

A system call can be invoked by generating a software interrupt to interrupt
vector 128 (80h). The argument registers are mapped as follow:

* `arg0` maps to CPU register `eax`.
* `arg1` maps to CPU register `ebx`.
* `arg2` maps to CPU register `esi`.
* `arg3` maps to CPU register `edi`.

## SYSCALL/SYSRET (Fast AMD) Implementation

A system call can be invoked by executing the `SYSCALL` CPU instruction.

* `arg0` maps to CPU register `eax`.
* `arg1` maps to CPU register `ebx`.
* `arg2` maps to CPU register `esi`.
* `arg3` maps to CPU register `edi`.

This system call is not supported by all CPUs.

## SYSENTER/SYSEXIT (Fast Intel) Implementation

A system call can be invoked by executing the `SYSENTER` CPU instruction.

* `arg0` maps to CPU register `eax`.
* `arg1` maps to CPU register `ebx`.
* `arg2` maps to CPU register `esi`.
* `arg3` maps to CPU register `edi`.
* The return address must be set in the `ecx` CPU register.
* The user stack pointer must be set in the `ebp` CPU register.

This system call is not supported by all CPUs.
