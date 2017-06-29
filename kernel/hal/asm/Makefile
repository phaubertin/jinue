jinue_root = ../../..
include $(jinue_root)/header.mk

STARTUP_SOURCES	 = startup.asm
TARGET_SOURCES	 = \
	frame_pointer.asm \
	io.asm \
	irq.asm \
	syscall.asm \
	thread.asm \
	x86.asm \

NASM_SOURCES	 = $(TARGET_SOURCES) $(STARTUP_SOURCES)

STARTUP			 = $(STARTUP_SOURCES:%.asm=%.o)
TARGET 			 = kern-hal-asm.o
TARGET_OBJS		 = $(TARGET_SOURCES:%.asm=%.o)

TARGETS			 = $(TARGET) $(STARTUP)

include $(common)

$(TARGET): $(TARGET_OBJS)
	$(LINK.o) -r $^ $(LOADLIBES) $(LDLIBS) -o $@
