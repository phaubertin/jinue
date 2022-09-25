#ifndef HAL_SERIAL_H_
#define HAL_SERIAL_H_

#include <hal/asm/serial.h>


void serial_init(int base_ioport, unsigned int baud_rate);

void serial_printn(int base_ioport, const char *message, unsigned int n);

void serial_putc(int base_ioport, char c);

#endif
