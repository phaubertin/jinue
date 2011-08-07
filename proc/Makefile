includes = ../include
scripts  = ../scripts
kernel   = ../kernel

OBJS  = proc.o syscall.o vga.o
KOBJS = $(kernel)/printk.o

CFLAGS  += -O3 -nostdinc -ffreestanding -static -ansi -Wall -Werror=implicit-function-declaration -Werror=uninitialized
LDFLAGS += -static -nostdlib -e main

.PHONY: all
all: proc

.PHONY: clean
clean:
	-rm -f proc $(OBJS)

$(KOBJS):
	make -C ../kernel

proc: $(OBJS) $(KOBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c -I$(includes) -o $@ $<

%.o: %.asm
	nasm -f elf -o $@ $<

