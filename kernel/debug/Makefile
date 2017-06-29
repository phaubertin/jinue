jinue_root = ../..
include $(jinue_root)/header.mk

C_SOURCES		 = debug.c kbd.c panic.c
TARGET			 = kern-debug.o

include $(common)

$(TARGET): $(OBJS)
	$(LINK.o) -r $^ $(LOADLIBES) $(LDLIBS) -o $@

