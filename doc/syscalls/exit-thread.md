# EXIT_THREAD - Exit the Current Thread

## Description

Exit the current thread.

## Arguments

Function number (`arg0`) is 12.

## Return Value

This function exits the current thread and does not return.

## Errors

This function always succeeds.

## Future Direction

The current system call only affects the current thread. It may be changed or
some other function may be added to allow another thread to be destroyed, in
which case the thread would be specified using a descriptor.
