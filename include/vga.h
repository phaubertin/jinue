#ifndef _JINUE_VGA_H_
#define _JINUE_VGA_H_

#define VGA_TEXT_VID_BASE	0xb8000
#define VGA_MISC_OUT_WR		0x3c2
#define VGA_MISC_OUT_RD		0x3cc
#define VGA_CRTC_ADDR		0x3d4
#define VGA_CRTC_DATA		0x3d5

#define VGA_FB_FLAG_ACTIVE	1

#define VGA_COLOR_BLACK			0x00
#define VGA_COLOR_BLUE			0x01
#define VGA_COLOR_GREEN			0x02
#define VGA_COLOR_CYAN			0x03
#define VGA_COLOR_RED			0x04
#define VGA_COLOR_MAGENTA		0x05
#define VGA_COLOR_BROWN			0x06
#define VGA_COLOR_WHITE			0x07
#define VGA_COLOR_GRAY			0x08
#define VGA_COLOR_BRIGHTBLUE	0x09
#define VGA_COLOR_BRIGHTGREEN	0x0a
#define VGA_COLOR_BRIGHTCYAN	0x0b
#define VGA_COLOR_BRIGHTRED		0x0c
#define VGA_COLOR_BRIGHTMAGENTA	0x0d
#define VGA_COLOR_YELLOW		0x0e
#define VGA_COLOR_BRIGHTWHITE	0x0f
#define VGA_COLOR_DEFAULT		VGA_COLOR_GREEN	
#define VGA_COLOR_ERASE			VGA_COLOR_RED

#define VGA_LINES		25
#define VGA_WIDTH		80
#define VGA_TAB_WIDTH	8

#define VGA_LINE(x) ((x) / (VGA_WIDTH))
#define VGA_COL(x) ((x) % (VGA_WIDTH))

typedef unsigned short vga_pos_t;

void vga_init(void);
void vga_clear(void);
void vga_print(const char *message);
void vga_printn(const char *message, unsigned int n);
void vga_putc(char c);
void vga_scroll(void);
vga_pos_t vga_get_cursor_pos(void);
void vga_set_cursor_pos(vga_pos_t pos);
vga_pos_t vga_raw_putc(char c, vga_pos_t pos);

#endif
