#ifndef _JINUE_DESCRIPTORS_H_
#define _JINUE_DESCRIPTORS_H_

/** GDT entry for the null descriptor */
#define GDT_NULL        		 0

/** GDT entry for kernel code segment */
#define GDT_KERNEL_CODE 		 1

/** GDT entry for kernel data segment */
#define GDT_KERNEL_DATA 		 2

/** GDT entry for user code segment */
#define GDT_USER_CODE   		 3

/** GDT entry for user data segment */
#define GDT_USER_DATA   		 4

/** GDT entry for task-state segment (TSS) */
#define GDT_TSS			   		 5

/** GDT entry for per-cpu data (includes the TSS) */
#define GDT_PER_CPU_DATA  		 6

/** GDT entry for thread-local storage */
#define GDT_USER_TLS_DATA	     7

/** number of descriptors in GDT */
#define GDT_LENGTH				 8

#define SEG_SELECTOR(index, rpl) \
	( ((index) << 3) | ((rpl) & 0x3) )

#endif
