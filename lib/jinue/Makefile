includes = ../../include

TARGET-USER = libjinue.a
TARGET-PROC = libjinue-proc.a
TARGET-KERN = libjinue-kernel.a

TARGETS     = $(TARGET-USER) $(TARGET-KERN) $(TARGET-PROC)

OBJS-USER   = stubs.o syscall.o
OBJS-PROC   = page_tables.o printk.o vga.o
OBJS-KERN   = page_tables.o printk.o
OBJS        = $(OBJS-USER) $(OBJS-PROC)

CFLAGS  += -O3 -nostdinc -ffreestanding -static -ansi -Wall -Werror=implicit-function-declaration -Werror=uninitialized

.PHONY: all
all: $(TARGETS)

.PHONY: all-kernel
all-kernel: $(TARGET-KERN)

.PHONY: all-proc
all-proc: $(TARGET-USER) $(TARGET-PROC)

.PHONY: clean
clean:
	-rm -f $(TARGETS) $(OBJS)

$(TARGET-USER): $(OBJS-USER)

$(TARGET-PROC): $(OBJS-PROC)

$(TARGET-KERN): $(OBJS-KERN)

%.a:
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -I$(includes) -I$(includes)/kstdc -o $@ $<

%.o: %.asm
	nasm -f elf -o $@ $<
