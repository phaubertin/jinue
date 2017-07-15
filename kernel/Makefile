jinue_root = ..
include $(jinue_root)/header.mk

subdirs			 = core debug hal kstdc mem

LDFLAGS         += -T $(scripts)/kernel.lds

kernel.startup	 = hal/asm/startup.o
kernel.core   	 = core/kern-core.o
kernel.debug  	 = debug/kern-debug.o
kernel.hal    	 = hal/kern-hal.o
kernel.lib    	 = ../lib/jinue/libjinue-kernel.a
kernel.mem    	 = mem/kern-mem.o
kernel.stdc   	 = kstdc/kern-stdc.o

target	 		 = kernel
objects.target	 = \
	$(kernel.startup) \
	$(kernel.core) \
	$(kernel.debug) \
	$(kernel.hal) \
    $(kernel.lib) \
    $(kernel.mem) \
    $(kernel.stdc) \

include $(common)

$(target): $(objects.target) $(scripts)/kernel.lds
	$(LINK.o) -r $(objects.target) $(LOADLIBES) $(LDLIBS) -o $@

.PHONY: core
core:
	make -C $@

.PHONY: debug
debug:
	make -C $@

.PHONY: hal
hal:
	make -C $@

.PHONY: kstdc
kstdc:
	make -C $@

.PHONY: mem
mem:
	make -C $@

.PHONY: libjinue
libjinue:
	make -C $(libjinue) all-kernel

# The core directory must be re-maked every build because of the auto-generated
# information about the build date and time.
$(kernel.core): core

$(kernel.debug): debug

$(kernel.hal) $(kernel.startup): hal

$(kernel.lib): libjinue

$(kernel.mem): mem

$(kernel.stdc): kstdc
