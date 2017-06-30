jinue_root = ../..
include $(jinue_root)/header.mk

sources.c		 = debug.c kbd.c panic.c
target			 = kern-debug.o

include $(common)

$(target): $(objects)
	$(LINK.o) -r $^ $(LOADLIBES) $(LDLIBS) -o $@

