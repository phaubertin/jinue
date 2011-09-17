#ifndef _JINUE_KERNEL_UTIL_H_
#define _JINUE_KERNEL_UTIL_H_

#define OFFSET_OF(x, s)		( (unsigned int)(x) & ((s)-1) )

#define ALIGN_START(x, s)	( (unsigned int)(x) & ~((s)-1) )

#define ALIGN_END(x, s)	( OFFSET_OF(x, s) == 0 ? (unsigned int)(x) : ALIGN_START(x, s) + s  )

#endif
