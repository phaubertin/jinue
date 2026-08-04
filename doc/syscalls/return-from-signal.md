# RETURN_FROM_SIGNAL - Return from a Signal

## Description

Return from an asynchronous signal handler.

## Arguments

The function number (`arg0`) is 25.

The address of the context is passed in `arg1`. The context is an object of
CPU architecture-dependent type `jinue_ucontext_t`.

```
    +----------------------------------------------------------------+
    |                         function = 25                          |  arg0
    +----------------------------------------------------------------+
    31                                                               0
    
    +----------------------------------------------------------------+
    |                         context address                        |  arg1
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                          reserved (0)                          |  arg2
    +----------------------------------------------------------------+
    31                                                               0

    +----------------------------------------------------------------+
    |                          reserved (0)                          |  arg3
    +----------------------------------------------------------------+
    31                                                               0
```

## Return Value

On success, this function returns 0 (in `arg0`). On failure, this function
returns -1 and an error number is set (in `arg1`).
    
## Errors

* JINUE_EINVAL if the context address is invalid.
* JINUE_EINVAL if the content of the context invalid.

No error will occur if the orignal context set up by the kernel when the signal
was delivered is passed back unmodified.
