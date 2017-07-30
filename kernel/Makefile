jinue_root = ..
include $(jinue_root)/header.mk

subdirs			 = core debug hal kstdc mem

kernel_ldscript	 = $(scripts)/kernel.ld

LDFLAGS         += -T $(kernel_ldscript)

kernel.core   	 = core/kern-core.o
kernel.debug  	 = debug/kern-debug.o
kernel.hal    	 = hal/kern-hal.o
kernel.lib    	 = ../lib/jinue/libjinue-kernel.a
kernel.mem    	 = mem/kern-mem.o
kernel.stdc   	 = kstdc/kern-stdc.o

target	 		 = kernel-stripped
unclean.extra    = kernel
objects.kernel	 = \
	$(kernel.core) \
	$(kernel.debug) \
	$(kernel.hal) \
    $(kernel.lib) \
    $(kernel.mem) \
    $(kernel.stdc) \

include $(common)

.PHONY: scripts
scripts:
	$(MAKE) -C $(scripts)

$(kernel_ldscript): scripts

kernel: $(objects.kernel) $(kernel_ldscript)
	$(LINK.o) $(objects.kernel) $(LOADLIBES) $(LDLIBS) -o $@

$(target): kernel

.PHONY: core
core:
	$(MAKE) -C $@

.PHONY: debug
debug:
	$(MAKE) -C $@

.PHONY: hal
hal:
	$(MAKE) -C $@

.PHONY: kstdc
kstdc:
	$(MAKE) -C $@

.PHONY: mem
mem:
	$(MAKE) -C $@

.PHONY: libjinue
libjinue:
	$(MAKE) -C $(libjinue) all-kernel

# The core directory must be re-maked every build because of the auto-generated
# information about the build date and time.
$(kernel.core): core

$(kernel.debug): debug

$(kernel.hal): hal

$(kernel.lib): libjinue

$(kernel.mem): mem

$(kernel.stdc): kstdc
