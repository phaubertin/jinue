#ifndef JINUE_HAL_PFADDR_H
#define JINUE_HAL_PFADDR_H

#include <jinue-common/pfaddr.h>
#include <stdint.h>

/** convert an address in an integer to a page frame address */
#define ADDR_TO_PFADDR(x)   ( (pfaddr_t)(  (uint64_t)(x) >> PFADDR_SHIFT ) )

/** convert a pointer to a page frame address (early mappings) */
#define PTR_TO_PFADDR(x)    ( (pfaddr_t)( (uintptr_t)(x) >> PFADDR_SHIFT ) )

/** convert a page frame address to a pointer (early mappings) */
#define PFADDR_TO_PTR(x)    ( (void *)( (x) << PFADDR_SHIFT ) )

/** ensure page frame address is valid (eight LSBs zero) */
#define PFADDR_CHECK(x)     ( ( (uint32_t)(x) << (32 - PAGE_SHIFT + PFADDR_SHIFT) ) == 0 )

/** check is the page frame address is below the 4GB (32-bit) limit */
#define PFADDR_CHECK_4GB(x) ( ( (uint32_t)(x) >> (32 - PFADDR_SHIFT) ) == 0 )

#endif
