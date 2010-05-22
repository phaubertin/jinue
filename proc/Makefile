includes = ../include
scripts  = ../scripts

OBJS = proc.o

CFLAGS  += -nostdinc -ffreestanding -static -Wall -ansi
LDFLAGS += -static -nostdlib -e main

.PHONY: all
all: proc

.PHONY: clean
clean:
	-rm -f proc $(OBJS)

proc: $(OBJS)

%.o: %.asm
	nasm -f elf -o $@ $<

