jinue_root = ../..
include $(jinue_root)/header.mk

OBJS   = core.o elf.o syscall.o
TARGET = kern-core.o

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	-rm -f $(TARGET) $(OBJS)

$(TARGET): $(OBJS)
	$(LINK.o) -r $^ $(LOADLIBES) $(LDLIBS) -o $@

core.o:        core.c        $(includes)/hal/hal.h build-info.h
elf.o:         elf.c         $(includes)/elf.h
syscall.o:     syscall.c     $(includes)/syscall.h

build-info.h: FORCE
	$(scripts)/gen_build_info_h.sh > $@

include $(jinue_root)/footer.mk
