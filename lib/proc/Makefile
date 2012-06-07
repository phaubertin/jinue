includes    = ../../include
kern-debug  = ../../kernel/debug

OBJS   = vga.o
PRINTK = $(kern-debug)/printk.o
TARGET = libproc.a

CFLAGS  += -O3 -nostdinc -ffreestanding -static -ansi -Wall -Werror=implicit-function-declaration -Werror=uninitialized -Werror=return-type

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	-rm -f $(TARGET) $(OBJS)

$(PRINTK):
	make -C $(kern-debug)

$(TARGET): $(PRINTK) $(OBJS)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -I$(includes) -I$(includes)/kstdc -o $@ $<

%.o: %.asm
	nasm -f elf -o $@ $<

