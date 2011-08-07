#include <jinue/ipc.h>
#include <jinue/syscall.h>
#include <stddef.h>
#include "syscall.h"


void vga_printn(const char *message, unsigned int n) {
	syscall_intr(SYSCALL_IPC_REF, SYSCALL_FUNCT_VGA_PUTS, (unsigned int)message, n);
}

void vga_putc(char c) {
	syscall_intr(SYSCALL_IPC_REF, SYSCALL_FUNCT_VGA_PUTS, (unsigned int)c, NULL);
}

void vga_print(const char *message) {
	size_t count;
	
	count = 0;
	
	while(message[count] != 0) {
		++count;
	}
	
	vga_printn(message, count);
}
