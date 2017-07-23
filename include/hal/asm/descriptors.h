#ifndef JINUE_HAL_ASM_DESCRIPTORS_H
#define JINUE_HAL_ASM_DESCRIPTORS_H


/** GDT entry for the null descriptor */
#define GDT_NULL                 0

/** GDT entry for kernel code segment */
#define GDT_KERNEL_CODE          1

/** GDT entry for kernel data segment */
#define GDT_KERNEL_DATA          2

/** GDT entry for user code segment */
#define GDT_USER_CODE            3

/** GDT entry for user data segment */
#define GDT_USER_DATA            4

/** GDT entry for task-state segment (TSS) */
#define GDT_TSS                  5

/** GDT entry for per-cpu data (includes the TSS) */
#define GDT_PER_CPU_DATA         6

/** GDT entry for thread-local storage */
#define GDT_USER_TLS_DATA        7

/** number of descriptors in GDT */
#define GDT_LENGTH               8

/** offset of descriptor type in descriptor */
#define SEG_FLAGS_OFFSET            40

/** size of the task-state segment (TSS) */
#define TSS_LIMIT                   104

/** segment is present */
#define SEG_FLAG_PRESENT            (1<<7)

/** system segment (i.e. call-gate, etc.) */
#define SEG_FLAG_SYSTEM             0

/** code/data/stack segment */
#define SEG_FLAG_NOSYSTEM           (1<<4)

/** 32-bit segment */
#define SEG_FLAG_32BIT              (1<<14)

/** 16-bit segment */
#define SEG_FLAG_16BIT              0

/** 32-bit gate */
#define SEG_FLAG_32BIT_GATE         (1<<3)

/** 16-bit gate */
#define SEG_FLAG_16BIT_GATE         0

/** task is busy (for TSS descriptor) */
#define SEG_FLAG_BUSY               (1<<1)

/** limit has page granularity */
#define SEG_FLAG_IN_PAGES           (1<<15)

/** limit has byte granularity */
#define SEG_FLAG_IN_BYTES           0

/** kernel/supervisor segment (privilege level 0) */
#define SEG_FLAG_KERNEL             0

/** user segment (privilege level 3) */
#define SEG_FLAG_USER               (3<<5)

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
#define SEG_TYPE_READ_ONLY           0

/** read/write data segment */
#define SEG_TYPE_DATA                2

/** task gate */
#define SEG_TYPE_TASK_GATE           5

/** interrupt gate */
#define SEG_TYPE_INTERRUPT_GATE      6

/** trap gate */
#define SEG_TYPE_TRAP_GATE           7

/** task-state segment (TSS) */
#define SEG_TYPE_TSS                 9

/** code segment */
#define SEG_TYPE_CODE               10

/** call gate */
#define SEG_TYPE_CALL_GATE          12


#endif
