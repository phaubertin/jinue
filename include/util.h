#ifndef JINUE_KERNEL_UTIL_H
#define JINUE_KERNEL_UTIL_H

#include <stddef.h>
#include <types.h>


#define OFFSET_OF(x, s)     ( (uint32_t)(x) & ((s)-1) )

#define ALIGN_START(x, s)   ((addr_t)( (uint32_t)(x) & ~((s)-1) ))

#define ALIGN_END(x, s)     ( OFFSET_OF(x, s) == 0 ? (addr_t)(x) : ALIGN_START(x, s) + s  )

static inline void *alloc_forward_func(size_t size, void **alloc_ptr) {
	char *ret = *alloc_ptr;

	*alloc_ptr = ret + size;

	return ret;
}

static inline void *alloc_backward_func(size_t size, void **alloc_ptr) {
	char *ret = *alloc_ptr;

	*alloc_ptr = ret - size;

	return *alloc_ptr;
}

#define alloc_forward(T, p) ((T *)alloc_forward_func(sizeof(T), &(p)))

#define alloc_backward(T, p) ((T *)alloc_forward_func(sizeof(T), &(p)))

#endif
