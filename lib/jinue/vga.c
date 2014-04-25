#include <jinue/syscall.h>
#include <stddef.h>

void vga_printn(const char *message, unsigned int n) {
    syscall(NULL, NULL, SYSCALL_FUNCT_VGA_PUTS, (unsigned int)message, n);
}

void vga_putc(char c) {
    syscall(NULL, NULL, SYSCALL_FUNCT_VGA_PUTC, (unsigned int)c, NULL);
}

void vga_print(const char *message) {
    size_t count;
    
    count = 0;
    
    while(message[count] != 0) {
        ++count;
    }
    
    vga_printn(message, count);
}
