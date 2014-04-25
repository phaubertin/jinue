#include <panic.h>
#include <printk.h>

#ifndef NDEBUG
void __assert_failed(
    const char *expr, 
    const char *file, 
    unsigned int line, 
    const char *func ) {    
    
    printk( 
        "ASSERTION FAILED [%s]: %s at line %u in function %s.\n",
        expr, file, line, func );

    panic("Assertion failed.");
}
#endif

