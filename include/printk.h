#ifndef _JINUE_KERNEL_PRINTK_H_
#define _JINUE_KERNEL_PRINTK_H_

void printk(const char *format, ...);

void print_unsigned_int(unsigned int n);

void print_hex_nibble(unsigned char byte);
void print_hex_b(unsigned char byte);
void print_hex_w(unsigned short word);
void print_hex_l(unsigned long dword);
void print_hex_q(unsigned long long qword);

#endif
