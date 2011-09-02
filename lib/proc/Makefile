includes = ../../include
kernel   = ../../kernel

OBJS   = vga.o
KOBJS  = $(kernel)/printk.o
TARGET = libproc.a

CFLAGS  += -O3 -nostdinc -ffreestanding -static -ansi -Wall -Werror=implicit-function-declaration -Werror=uninitialized

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	-rm -f $(TARGET) $(OBJS)

$(KOBJS):
	make -C $(kernel)

$(TARGET): $(KOBJS) $(OBJS)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -I$(includes) -I$(includes)/kstdc -o $@ $<

%.o: %.asm
	nasm -f elf -o $@ $<

