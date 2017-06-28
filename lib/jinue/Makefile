includes = ../../include

TARGET-USER = libjinue.a
TARGET-PROC = libjinue-proc.a
TARGET-KERN = libjinue-kernel.a

TARGETS     = $(TARGET-USER) $(TARGET-KERN) $(TARGET-PROC)

OBJS-USER   = crt.o stubs.o syscall.o
OBJS-PROC   = page_tables.o printk.o vga.o
OBJS-KERN   = page_tables.o printk.o
OBJS        = $(OBJS-USER) $(OBJS-PROC)

CPPFLAGS	+= -I$(includes) -I$(includes)/kstdc -nostdinc
CFLAGS  	+= -m32 -march=i686 -O3 -nostdinc -ffreestanding -static -std=c99 -pedantic -Wall -Werror=implicit-function-declaration -Werror=uninitialized

.PHONY: all
all: $(TARGETS)

.PHONY: all-kernel
all-kernel: $(TARGET-KERN)

.PHONY: all-proc
all-proc: $(TARGET-USER) $(TARGET-PROC)

.PHONY: clean
clean:
	-rm -f $(TARGETS) $(OBJS) $(OBJS:%.o=%.nasm)

$(TARGET-USER): $(OBJS-USER)

$(TARGET-PROC): $(OBJS-PROC)

$(TARGET-KERN): $(OBJS-KERN)

%.a:
	ar rcs $@ $^

%.nasm: %.asm
	$(CPP) $(CPPFLAGS) -x assembler-with-cpp -o $@ $<

%.o: %.nasm
	nasm -f elf -o $@ $<
