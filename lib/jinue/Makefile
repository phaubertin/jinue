includes = ../../include

OBJS   = stubs.o syscall.o
TARGET = libjinue.a

CFLAGS  += -O3 -nostdinc -ffreestanding -static -ansi -Wall -Werror=implicit-function-declaration -Werror=uninitialized

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	-rm -f $(TARGET) $(OBJS)

$(TARGET): $(OBJS)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -I$(includes) -o $@ $<

%.o: %.asm
	nasm -f elf -o $@ $<

