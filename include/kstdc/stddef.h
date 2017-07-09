#ifndef _JINUE_KSTDC_STDDEF_H
#define _JINUE_KSTDC_STDDEF_H

typedef signed long     ptrdiff_t;
typedef unsigned long   size_t;
typedef int             wchar_t;

#ifndef NULL
#define NULL 0
#endif

#define offsetof(type, member) ( (size_t) &( ((type *)0)->member ) )

#endif

