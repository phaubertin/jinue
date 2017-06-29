jinue_root = ../..
include $(jinue_root)/header.mk

C_SOURCES	 = assert.c string.c
TARGET		 = kern-stdc.o

include $(common)

$(TARGET): $(OBJS)
	$(LINK.o) -r $^ $(LOADLIBES) $(LDLIBS) -o $@
