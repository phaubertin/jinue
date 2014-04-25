#include <hal/io.h>
#include <printk.h>
#include <stdbool.h>

void any_key(void) {
    unsigned char buffer;
    bool ignore;
    
    /* prompt */
    printk("(press enter)");
    
    /* wait for key, ignore break codes */
    ignore = false;
    while(1) {
        do {
            buffer = inb(0x64);
        } while( (buffer & 1) == 0 );
        
        buffer = inb(0x60);
        
        if(buffer == 0x0e || buffer == 0x0f) {
            ignore = true;
            continue;
        }        
        
        if(ignore) {
            ignore = false;
            continue;
        }
        
        if(buffer == 0x1c || buffer == 0x5a) {
            break;
        }
    }
    
    /* advance cursor */
    printk("\n");
}
