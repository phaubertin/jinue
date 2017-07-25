#ifndef _JINUE_COMMON_PFADDR_H
#define _JINUE_COMMON_PFADDR_H

#include <jinue-common/types.h>
#include <jinue-common/vm.h>


/** number of bits by which the page frame address is shifted to the right */
#define PFADDR_SHIFT        PAGE_SHIFT

/** an invalid page frame address used as null value */
#define PFNULL              ((pfaddr_t)-1)

#endif
