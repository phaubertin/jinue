jinue_root = ../..
include $(jinue_root)/header.mk

sources.c		 = pfalloc.c slab.c vm_alloc.c
target			 = kern-mem.o

include $(common)

$(target): $(objects)
	$(LINK.o) -r $^ $(LOADLIBES) $(LDLIBS) -o $@
