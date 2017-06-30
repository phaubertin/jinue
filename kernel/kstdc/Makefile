jinue_root = ../..
include $(jinue_root)/header.mk

sources.c	 = assert.c string.c
target		 = kern-stdc.o

include $(common)

$(target): $(objects)
	$(LINK.o) -r $^ $(LOADLIBES) $(LDLIBS) -o $@
