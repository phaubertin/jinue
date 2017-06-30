jinue_root = ../../..
include $(jinue_root)/header.mk

sources.startup	 = startup.asm
sources.target	 = \
	frame_pointer.asm \
	io.asm \
	irq.asm \
	syscall.asm \
	thread.asm \
	x86.asm \

sources.nasm     = $(sources.target) $(sources.startup)

startup          = $(sources.startup:%.asm=%.o)
target           = kern-hal-asm.o
objects.target   = $(sources.target:%.asm=%.o)

targets          = $(target) $(startup)

include $(common)

$(target): $(objects.target)
	$(LINK.o) -r $^ $(LOADLIBES) $(LDLIBS) -o $@
