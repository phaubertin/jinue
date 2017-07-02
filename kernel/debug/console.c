#include <jinue/console.h>
#include <hal/vga.h>
#include <stddef.h>

void console_printn(const char *message, unsigned int n) {
    vga_printn(message, n);
}

void console_putc(char c) {
    vga_putc(c);
}

void console_print(const char *message) {
    size_t count;

    count = 0;

    while(message[count] != 0) {
        ++count;
    }

    console_printn(message, count);
}
