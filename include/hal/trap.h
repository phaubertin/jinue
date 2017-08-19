#ifndef JINUE_HAL_TRAP_H
#define JINUE_HAL_TRAP_H


extern int syscall_method;

/** entry point for Intel fast system call mechanism (SYSENTER/SYSEXIT) */
void fast_intel_entry(void);

/** entry point for AMD fast system call mechanism (SYSCALL/SYSRET) */
void fast_amd_entry(void);

/* do not call - used by new user threads to "return" to user space for the
 * first time. See thread_page_create(). */
void return_from_interrupt(void);

#endif
