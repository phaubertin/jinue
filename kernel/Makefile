jinue_root = ..
include $(jinue_root)/header.mk

subdirs			 = core debug hal kstdc mem

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
	$(LINK.o) -r $^ $(LOADLIBES) $(LDLIBS) -o $@

$(kernel.startup):
	make -C hal hal-startup

# The core directory must be re-maked every build because of the auto-generated
# information about the build date and time.
$(kernel.core): FORCE
	make -C core
	
$(kernel.debug):
	make -C debug
	
$(kernel.hal):
	make -C hal

$(kernel.lib):
	make -C ../lib/jinue all-kernel
	
$(kernel.mem):
	make -C mem

$(kernel.stdc):
	make -C kstdc
