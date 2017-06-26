#ifndef _JINUE_KERNEL_IO_H_
#define _JINUE_KERNEL_IO_H_

#include <stdint.h>

uint8_t  inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);

void outb(uint16_t port, uint8_t  value);
void outw(uint16_t port, uint16_t value);
void outl(uint16_t port, uint32_t value);

#endif

