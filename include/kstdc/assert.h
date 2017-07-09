#ifndef _JINUE_KSTDC_ASSERT_H
#define _JINUE_KSTDC_ASSERT_H

#undef assert

#ifdef NDEBUG
#define assert(ignore) ((void)0)
#else
    void __assert_failed(
        const char *expr, 
        const char *file, 
        unsigned int line, 
        const char *func );
    
#define assert(expr) \
    ( \
        (expr)?(void)0:( __assert_failed(#expr, __FILE__, __LINE__, __func__) ) \
    )
#endif

#endif
