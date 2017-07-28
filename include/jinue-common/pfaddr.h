#ifndef _JINUE_COMMON_PFADDR_H
#define _JINUE_COMMON_PFADDR_H

#include <jinue-common/vm.h>

/** type for a page frame address (32-bit value) */
typedef uint32_t pfaddr_t;

/** number of bits by which the page frame address is shifted to the right */
#define PFADDR_SHIFT        PAGE_BITS

/** an invalid page frame address used as null value */
#define PFNULL              ((pfaddr_t)-1)

#endif
