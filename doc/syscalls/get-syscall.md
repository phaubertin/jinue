# GET_SYSCALL - Get System Call Mechanism

## Description

Get the best supported system call mechanism.

## Arguments

Function number (`arg0`) is 1.

## Return Value

Return value (`arg0`) identifies the [system call mechanism](mechanisms.md):

* 0 for SYSENTER/SYSEXIT (Fast Intel) Mechanism
* 1 for SYSCALL/SYSRET (Fast AMD) Mechanism
* 2 for Interrupt-Based Mechanism

## Errors

This function always succeeds.

## Future Direction

This system call may be removed in the future, with the value it returns passed
in auxiliary vectors instead.
