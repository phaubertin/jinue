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
        int  val;
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

typedef void (*syscall_funct_t)(syscall_params_t *);

extern int syscall_method;

extern syscall_funct_t hal_syscall_funct;


void hal_syscall_dispatch(syscall_params_t *syscall_params);

void set_syscall_funct(syscall_funct_t syscall_funct);

void default_syscall_funct(syscall_params_t *syscall_params);

/** entry point for Intel fast sytem call mechanism (sysenter/sysexit) */
void fast_intel_entry(void);

/** entry point for AMD fast sytem call mechanism (syscall/sysret) */
void fast_amd_entry(void);

#endif
