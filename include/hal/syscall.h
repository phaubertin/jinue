#ifndef _JINUE_KERNEL_HAL_SYSCALL_H_
#define _JINUE_KERNEL_HAL_SYSCALL_H_

#include <jinue/syscall.h>


/** system call parameters */
typedef union {
    struct {
        syscall_ref_t   dest;
        unsigned int    method;
        unsigned int    funct;
        unsigned int    arg1;
        unsigned int    arg2;
    } args;
    struct {
        int  errno;
        int  reserved1;
        int  retval;
        int  reserved2;
        int *perrno;
    } ret;
    struct {
        unsigned int ebx;
        unsigned int edx;
        unsigned int eax;
        unsigned int esi;
        unsigned int edi;        
    } regs;
} syscall_params_t;

extern int syscall_method;

/** entry point for Intel fast sytem call mechanism (sysenter/sysexit) */
void fast_intel_entry(void);

/** entry point for AMD fast sytem call mechanism (syscall/sysret) */
void fast_amd_entry(void);

void dispatch_syscall(syscall_params_t *);

#endif
