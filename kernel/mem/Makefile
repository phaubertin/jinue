jinue_root = ../..
include $(jinue_root)/header.mk

OBJS   = pfalloc.o slab.o vm_alloc.o
TARGET = kern-mem.o

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	-rm -f $(TARGET) $(OBJS)

$(TARGET): $(OBJS)
	$(LINK.o) -r $^ $(LOADLIBES) $(LDLIBS) -o $@

pfalloc.o:     pfalloc.c     $(includes)/pfalloc.h
slab.o:        slab.c        $(includes)/slab.h
vm_alloc.o:    vm_alloc.c    $(includes)/vm_alloc.h

include $(jinue_root)/footer.mk
