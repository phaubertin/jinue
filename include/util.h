#ifndef _JINUE_KERNEL_UTIL_H_
#define _JINUE_KERNEL_UTIL_H_

#include <jinue/types.h>

#define OFFSET_OF(x, s)		( (uint32_t)(x) & ((s)-1) )

#define ALIGN_START(x, s)	((addr_t)( (uint32_t)(x) & ~((s)-1) ))

#define ALIGN_END(x, s)	    ( OFFSET_OF(x, s) == 0 ? (addr_t)(x) : ALIGN_START(x, s) + s  )

#endif
