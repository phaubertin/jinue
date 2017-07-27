jinue_root = ../..
include $(jinue_root)/header.mk

subdirs = asm

sources.c    = \
	boot.c \
	bootmem.c \
	cpu.c \
	e820.c \
	hal.c \
	interrupt.c \
	thread.c \
	vga.c \
	vm.c \
	vm_x86.c \

hal.asm     = asm/kern-hal-asm.o
target      = kern-hal.o

include $(common)

.PHONY: asm
asm:
	make -C asm

$(hal.asm): asm

$(target): $(objects) $(hal.asm)
	$(LINK.o) -r $^ $(LOADLIBES) $(LDLIBS) -o $@
