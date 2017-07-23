#ifndef JINUE_HAL_DESCRIPTORS_H
#define JINUE_HAL_DESCRIPTORS_H

#include <hal/asm/descriptors.h>

#include <stdint.h>
#include <types.h>


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

#define SEG_SELECTOR(index, rpl) \
    ( ((index) << 3) | ((rpl) & 0x3) )

#endif
