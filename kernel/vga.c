#include <vga.h>
#include <io.h>

void vga_init(void) {
	unsigned char data;

	/* Set address select bit in a known state: CRTC regs at 0x3dx */
	data = inb(VGA_MISC_OUT_RD);
	data |= 1;
	outb(VGA_MISC_OUT_WR, data);

	/* Move cursor to line 0 col 0 */
	outb(VGA_CRTC_ADDR, 0x0e);
	outb(VGA_CRTC_DATA, 0x0);	
	outb(VGA_CRTC_ADDR, 0x0f);
	outb(VGA_CRTC_DATA, 0x0);
	
	/* Clear the screen */
	vga_clear();
}

void vga_clear(void) {
	unsigned char *buffer = (unsigned char *)VGA_TEXT_VID_BASE;
	unsigned int idx = 0;
	
	while( idx < (VGA_LINES * VGA_WIDTH * 2) ) 	{
		buffer[idx++] = 0x20;
		buffer[idx++] = VGA_COLOR_ERASE;
	}
}

void vga_scroll(void) {
	unsigned char *di = (unsigned char *)VGA_TEXT_VID_BASE;
	unsigned char *si = (unsigned char *)(VGA_TEXT_VID_BASE + 2 * VGA_WIDTH);
	unsigned int idx;
	
	for(idx = 0; idx < 2 * VGA_WIDTH * (VGA_LINES - 1); ++idx) {	
		*(di++) = *(si++);
	}
	
	for(idx = 0; idx < VGA_WIDTH; ++idx) {
		*(di++) = 0x20;
		*(di++) = VGA_COLOR_ERASE;
	}
}

vga_pos_t vga_get_cursor_pos(void) {
	unsigned char h, l;
	
	outb(VGA_CRTC_ADDR, 0x0e);
	h = inb(VGA_CRTC_DATA);
	outb(VGA_CRTC_ADDR, 0x0f);
	l = inb(VGA_CRTC_DATA);
	
	return (h << 8) | l;
}

void vga_set_cursor_pos(vga_pos_t pos) {
	unsigned char h = pos >> 8;
	unsigned char l = pos;
	
	outb(VGA_CRTC_ADDR, 0x0e);
	outb(VGA_CRTC_DATA, h);
	outb(VGA_CRTC_ADDR, 0x0f);
	outb(VGA_CRTC_DATA, l);	
}


void vga_print(const char *message) {
	unsigned short int pos = vga_get_cursor_pos();
	char c;
	
	while( c = *(message++) ) {
		pos = vga_raw_putc(c, pos);
	}
	
	vga_set_cursor_pos(pos);
}

void vga_printn(const char *message, unsigned int n) {
	unsigned short int pos = vga_get_cursor_pos();
	char c;
	
	while(n) {
		c = *(message++);
		pos = vga_raw_putc(c, pos);
		--n;
	}
	
	vga_set_cursor_pos(pos);
}

void vga_putc(char c) {
	unsigned short int pos = vga_get_cursor_pos();

	pos = vga_raw_putc(c, pos);
	
	vga_set_cursor_pos(pos);
}

vga_pos_t vga_raw_putc(char c, vga_pos_t pos) {
	const vga_pos_t limit = VGA_WIDTH * VGA_LINES;
	char *buffer = (char *)VGA_TEXT_VID_BASE;
	
	switch(c) {
	/* backspace */
	case 0x08:
		if( VGA_COL(pos) > 0 ) {
			--pos;
		}
		break;
	
	/* linefeed */
	case 0x0a:
		pos = VGA_WIDTH * (VGA_LINE(pos) + 1);
		break;
	
	/* carriage return */
	case 0x0d:
		pos = VGA_WIDTH * VGA_LINE(pos);
		break;
	
	/* tab */
	case 0x09:
		pos -= pos % VGA_TAB_WIDTH;
		pos += VGA_TAB_WIDTH;
		break;
	
	default:
		if(c >= 0x20) {
			buffer[2*pos] = c;
			buffer[2*pos+1] =  VGA_COLOR_DEFAULT;
			++pos;
		}
	}
	
	if(pos >= limit) {
		pos -= VGA_WIDTH;
		vga_scroll();
	}
	
	return pos;
}
