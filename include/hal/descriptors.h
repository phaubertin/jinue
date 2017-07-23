#ifndef JINUE_HAL_DESCRIPTORS_H
#define JINUE_HAL_DESCRIPTORS_H

#include <jinue/descriptors.h>
#include <stdint.h>
#include <types.h>


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


typedef uint64_t seg_descriptor_t;

typedef uint32_t seg_selector_t;

typedef struct {
    uint16_t padding;
    uint16_t limit;
    addr_t         addr;
} pseudo_descriptor_t;

typedef struct {
    /* offset 0 */
    uint16_t prev;
    /* offset 4 */
    addr_t         esp0;    
    /* offset 8 */
    uint16_t ss0;
    /* offset 12 */
    addr_t         esp1;    
    /* offset 16 */
    uint16_t ss1;
    /* offset 20 */
    addr_t         esp2;    
    /* offset 24 */
    uint16_t ss2;
    /* offset 28 */
    uint32_t  cr3;
    /* offset 32 */
    uint32_t  eip;
    /* offset 36 */
    uint32_t  eflags;
    /* offset 40 */
    uint32_t  eax;
    /* offset 44 */
    uint32_t  ecx;
    /* offset 48 */
    uint32_t  edx;
    /* offset 52 */
    uint32_t  ebx;
    /* offset 56 */
    uint32_t  esp;
    /* offset 60 */
    uint32_t  ebp;
    /* offset 64 */
    uint32_t  esi;
    /* offset 68 */
    uint32_t  edi;
    /* offset 72 */
    uint16_t es;
    /* offset 76 */
    uint16_t cs;
    /* offset 80 */
    uint16_t ss;
    /* offset 84 */
    uint16_t ds;
    /* offset 88 */
    uint16_t fs;
    /* offset 92 */
    uint16_t gs;
    /* offset 96 */
    uint16_t ldt;
    /* offset 100 */
    uint16_t debug;
    uint16_t iomap;
} tss_t;

#define PACK_DESCRIPTOR(val, mask, shamt1, shamt2) \
    ( (((uint64_t)(uintptr_t)(val) >> shamt1) & mask) << shamt2 )

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

#endif
