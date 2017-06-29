jinue_root = ../..
include $(jinue_root)/header.mk

build_info_h	 = build-info.gen.h

C_SOURCES		 = core.c elf.c syscall.c
TARGET			 = kern-core.o

UNCLEAN			+= $(build_info_h)

include $(common)

$(TARGET): $(OBJS)
	$(LINK.o) -r $^ $(LOADLIBES) $(LDLIBS) -o $@

core.o: core.c $(build_info_h)

$(build_info_h): FORCE
	$(scripts)/gen_build_info_h.sh > $@
