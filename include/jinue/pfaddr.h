#ifndef _JINUE_PFADDR_H_
#define _JINUE_PFADDR_H_

#include <stdint.h>
#include <jinue/vm.h>


/** number of bits by which the page frame address is shifted to the right */
#define PFADDR_SHIFT	4

/** convert a pointer to a page frame address */
#define PTR_TO_PFADDR(x)	( (pfaddr_t)( (uintptr_t)(x) >> PFADDR_SHIFT ) )

/** convert an address in an integer to a page frame address */
#define ADDR_TO_PFADDR(x)	( (pfaddr_t)(  (uint64_t)(x) >> PFADDR_SHIFT ) )

/** ensure page frame address is valid (eight LSBs zero) */
#define PFADDR_CHECK(x)		( ( (uint32_t)(x) << (32 - PAGE_BITS + PFADDR_SHIFT) ) == 0 )

/** check is the page frame address is below the 4GB (32-bit) limit */
#define PFADDR_CHECK_4GB(x)	( ( (uint32_t)(x) >> (32 - PFADDR_SHIFT) ) == 0 )

/** type for a page frame address (32-bit value) */
typedef uint32_t pfaddr_t;


#endif

