jinue_root = ../..
include $(jinue_root)/header.mk

build_info_h     = build-info.gen.h

sources.c        = core.c elf.c syscall.c thread.c
target           = kern-core.o

unclean         += $(build_info_h)

include $(common)

$(target): $(objects)
	$(LINK.o) -r $^ $(LOADLIBES) $(LDLIBS) -o $@

core.o: core.c $(build_info_h)

$(build_info_h): FORCE
	$(scripts)/gen_build_info_h.sh > $@
