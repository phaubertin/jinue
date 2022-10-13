# YIELD_THREAD - Yield the Current Thread

## Description

Have the current thread relinquish the CPU and allow other ready threads to run.

## Arguments

Function number (`arg0`) is 5.

## Return Value

No return value.

## Errors

This function always succeeds.

## Future Direction

Currently, the only ways for a thread to allow other threads to run are to
either block waiting (e.g. sending or receiving a message) or call this
function. However, proper preemptive thread scheduling will be implemented.
