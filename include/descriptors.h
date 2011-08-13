#ifndef _JINUE_KERNEL_DESCRIPTORS_H_
#define _JINUE_KERNEL_DESCRIPTORS_H_

#include <kernel.h>

/** offset of descriptor type in descriptor */
#define SEG_FLAGS_OFFSET		40

/** size of the task-state segment (TSS) */
#define TSS_LIMIT				104

/** segment is present */
#define SEG_FLAG_PRESENT  		(1<<7)

/** system segment (i.e. call-gate, etc.) */
#define SEG_FLAG_SYSTEM   		 0

/** code/data/stack segment */
#define SEG_FLAG_NOSYSTEM 		(1<<4)

/** 32-bit segment */
#define SEG_FLAG_32BIT    		(1<<14)

/** 16-bit segment */
#define SEG_FLAG_16BIT    		 0

/** 32-bit gate */
#define SEG_FLAG_32BIT_GATE		(1<<3)

/** 16-bit gate */
#define SEG_FLAG_16BIT_GATE		 0

/** task is busy (for TSS descriptor) */
#define SEG_FLAG_BUSY			(1<<1)

/** limit has page granularity */
#define SEG_FLAG_IN_PAGES		(1<<15)

/** limit has byte granularity */
#define SEG_FLAG_IN_BYTES 	 	 0

/** kernel/supervisor segment (privilege level 0) */
#define SEG_FLAG_KERNEL   	 	 0

/** user segment (privilege level 3) */
#define SEG_FLAG_USER    		(3<<5)

/** commonly used segment flags */
#define SEG_FLAG_NORMAL \
	(SEG_FLAG_32BIT | SEG_FLAG_IN_PAGES | SEG_FLAG_NOSYSTEM | SEG_FLAG_PRESENT)

/** commonly used gate flags */
#define SEG_FLAG_NORMAL_GATE \
	(SEG_FLAG_32BIT_GATE | SEG_FLAG_SYSTEM | SEG_FLAG_PRESENT)

/** commonly used  flags for task-state segment */
#define SEG_FLAG_TSS \
	(SEG_FLAG_IN_BYTES | SEG_FLAG_SYSTEM | SEG_FLAG_PRESENT)


/** read-only data segment */
#define SEG_TYPE_READ_ONLY 	 	 0

/** read/write data segment */
#define SEG_TYPE_DATA      	 	 2

/** task gate */
#define SEG_TYPE_TASK_GATE   	 5

/** interrupt gate */
#define SEG_TYPE_INTERRUPT_GATE	 6

/** trap gate */
#define SEG_TYPE_TRAP_GATE   	 7

/** task-state segment (TSS) */
#define SEG_TYPE_TSS		   	 9

/** code segment */
#define SEG_TYPE_CODE      		10

/** call gate */
#define SEG_TYPE_CALL_GATE		12


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

/** GDT entry for task-state segment (TSS) */
#define GDT_TSS_DATA	   		 6

/** end of GDT / next-to-last entry */
#define GDT_END   				 7


typedef unsigned long long seg_descriptor_t;

typedef seg_descriptor_t *gdt_t;

typedef seg_descriptor_t *ldt_t;

typedef seg_descriptor_t *idt_t;

typedef unsigned long seg_selector_t;

typedef struct {
	unsigned short padding;
	unsigned short limit;
	gdt_t          addr;
} gdt_info_t;

typedef gdt_info_t idt_info_t;

typedef struct {
	/* offset 0 */
	unsigned short prev;	
	/* offset 4 */
	addr_t         esp0;	
	/* offset 8 */
	unsigned short ss0;	
	/* offset 12 */
	addr_t         esp1;	
	/* offset 16 */
	unsigned short ss1;	
	/* offset 20 */
	addr_t         esp2;	
	/* offset 24 */
	unsigned short ss2;	
	/* offset 28 */
	unsigned long  cr3;	
	/* offset 32 */
	unsigned long  eip;	
	/* offset 36 */
	unsigned long  eflags;	
	/* offset 40 */
	unsigned long  eax;	
	/* offset 44 */
	unsigned long  ecx;	
	/* offset 48 */
	unsigned long  edx;	
	/* offset 52 */
	unsigned long  ebx;	
	/* offset 56 */
	unsigned long  esp;	
	/* offset 60 */
	unsigned long  ebp;	
	/* offset 64 */
	unsigned long  esi;	
	/* offset 68 */
	unsigned long  edi;	
	/* offset 72 */
	unsigned short es;	
	/* offset 76 */
	unsigned short cs;	
	/* offset 80 */
	unsigned short ss;	
	/* offset 84 */
	unsigned short ds;	
	/* offset 88 */
	unsigned short fs;	
	/* offset 92 */
	unsigned short gs;	
	/* offset 96 */
	unsigned short ldt;	
	/* offset 100 */
	unsigned short debug;
	unsigned short iomap;	
} tss_t;

#define PACK_DESCRIPTOR(val, mask, shamt1, shamt2) \
	( (((unsigned long long)(val) >> shamt1) & mask) << shamt2 )

#define SEG_DESCRIPTOR(base, limit, type) (\
	  PACK_DESCRIPTOR((type),  0xf0ff,  0, SEG_FLAGS_OFFSET) \
	| PACK_DESCRIPTOR((base),  0xff,   24, 56) \
	| PACK_DESCRIPTOR((base),  0xff,   16, 32) \
	| PACK_DESCRIPTOR((base),  0xffff,  0, 16) \
	| PACK_DESCRIPTOR((limit), 0xf,    16, 48) \
	| PACK_DESCRIPTOR((limit), 0xffff,  0,  0) \
)

#define GATE_DESCRIPTOR(segment, offset, type, param_count) (\
	  PACK_DESCRIPTOR((type),        0xff,    0,  SEG_FLAGS_OFFSET) \
	| PACK_DESCRIPTOR((param_count), 0xf,     0,  32) \
	| PACK_DESCRIPTOR((segment),     0xffff,  0,  16) \
	| PACK_DESCRIPTOR((offset),      0xffff, 16,  48) \
	| PACK_DESCRIPTOR((offset),      0xffff,  0,   0) \
)

#define SEG_SELECTOR(index, rpl) \
	( ((index) << 3) | ((rpl) & 0x3) )

#endif
