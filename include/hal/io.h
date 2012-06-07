#ifndef _JINUE_KERNEL_IO_H_
#define _JINUE_KERNEL_IO_H_

unsigned char inb(unsigned short int port);
unsigned short int inw(unsigned short int port);
unsigned int inl(unsigned short int port);

void outb(unsigned short int port, unsigned char value);
void outw(unsigned short int port, unsigned short int value);
void outl(unsigned short int port, unsigned int value);

#endif

