#ifndef _JINUE_KERNEL_STARTUP_H_
#define _JINUE_KERNEL_STARTUP_H_

#define KERNEL_STACK_SIZE	8192

void __start(void);
void halt(void);

#endif

