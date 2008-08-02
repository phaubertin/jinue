#include <printk.h>
#include <stdarg.h>
#include <stddef.h>
#include <vga.h>

void printk(const char *format, ...) {
	va_list ap;
	const char *idx, *anchor;
	ptrdiff_t size;
	
	va_start(ap, format);
	
	idx = format;
	
	while(1) {
		anchor = idx;
		
		while( *idx != 0 && *idx != '%' ) {
			++idx;
		}
		
		size = idx - anchor;
		
		if(size > 0) {
			vga_printn(anchor, size);
		}
		
		if(*idx == 0 || *(idx+1) == 0) {
			break;
		}
		
		++idx;
		
		switch( *idx ) {
		case '%':
			vga_putc('%');
			break;
		
		case 's':
			vga_print( va_arg(ap, const char *) );
			break;
		
		case 'u':
			print_unsigned_int( va_arg(ap, unsigned int) );
			break;
		
		case 'x':
			print_hex_l( va_arg(ap, unsigned long) );
			break;
		
		default:
			va_end(ap);
			return;
		}
		
		++idx;
	}	
	
	va_end(ap);		
}

void print_unsigned_int(unsigned int n) {
	unsigned int flag = 0;
	unsigned int pwr;
	unsigned int digit;
	char c;
	
	if(n == 0) {
		vga_putc('0');
		return;
	}
	
	for(pwr = 1000 * 1000 * 1000; pwr > 0; pwr /= 10) {
		digit = n / pwr;
		
		if(digit != 0 || flag) {
			c = (char)digit + '0';
			vga_putc(c);
			
			flag = 1;
			n -= digit * pwr;
		}
	}	
}

void print_hex_nibble(unsigned char byte) {
	char c;
	
	c = byte & 0xf;
	if(c < 10) {
		c += '0';
	}
	else {
		c+= ('a' - 10);
	}
	
	vga_putc(c);
}

void print_hex_b(unsigned char byte) {
	print_hex_nibble( (char)byte );
	print_hex_nibble( (char)(byte>>4) );	
}

void print_hex_w(unsigned short word) {
	int off;
	
	for(off=16-4; off>=0; off-=4) {
		print_hex_nibble( (char)(word>>off) );
	}
}

void print_hex_l(unsigned long dword) {
	int off;
	
	for(off=32-4; off>=0; off-=4) {
		print_hex_nibble( (char)(dword>>off) );
	}
}

void print_hex_q(unsigned long long qword) {
	int off;
	
	for(off=64-4; off>=0; off-=4) {
		print_hex_nibble( (char)(qword>>off) );
	}
}

