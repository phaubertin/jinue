jinue_root = ../..
include $(jinue_root)/header.mk

C_SOURCES		 = pfalloc.c slab.c vm_alloc.c
TARGET			 = kern-mem.o

include $(common)

$(TARGET): $(OBJS)
	$(LINK.o) -r $^ $(LOADLIBES) $(LDLIBS) -o $@
