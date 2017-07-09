#ifndef _JINUE_DESCRIPTORS_H
#define _JINUE_DESCRIPTORS_H

#include <jinue/asm/descriptors.h>

#define SEG_SELECTOR(index, rpl) \
    ( ((index) << 3) | ((rpl) & 0x3) )

#endif
