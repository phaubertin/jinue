jinue_root = ../../..
include $(jinue_root)/header.mk

sources.nasm	 = \
	frame_pointer.asm \
	io.asm \
	irq.asm \
	startup.asm \
	syscall.asm \
	thread.asm \
	x86.asm \

target           = kern-hal-asm.o

include $(common)

$(target): $(objects)
	$(LINK.o) -r $^ $(LOADLIBES) $(LDLIBS) -o $@
