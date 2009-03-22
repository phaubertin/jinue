#ifndef _JINUE_KERNEL_STDARG_H
#define _JINUE_KERNEL_STDARG_H

typedef	__builtin_va_list	va_list;

#define va_start(ap, parmN)	__builtin_stdarg_start((ap), (parmN))
#define va_arg					__builtin_va_arg
#define va_end					__builtin_va_end
#define va_copy(dest, src)		__builtin_va_copy((dest), (src))

#endif
