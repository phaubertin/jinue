jinue_root = ../..
include $(jinue_root)/header.mk

subdirs = asm

C_SOURCES	 = \
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

OBJS = boot.o bootmem.o cpu.o e820.o hal.o interrupt.o thread.o vga.o vm.o vm_x86.o

HAL-ASM     = asm/kern-hal-asm.o
HAL-STARTUP = asm/startup.o
TARGET      = kern-hal.o

include $(common)

.PHONY: asm
asm:
	make -C asm

.PHONY: hal-startup
hal-startup : $(HAL-STARTUP)

$(HAL-STARTUP) $(HAL-ASM): asm

$(TARGET): $(OBJS) $(HAL-ASM)
	$(LINK.o) -r $^ $(LOADLIBES) $(LDLIBS) -o $@
