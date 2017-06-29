jinue_root = ..
include $(jinue_root)/header.mk

KERN-STARTUP = hal/asm/startup.o
KERN-CORE    = core/kern-core.o
KERN-DEBUG   = debug/kern-debug.o
KERN-HAL     = hal/kern-hal.o
KERN-LIB     = ../lib/jinue/libjinue-kernel.a
KERN-MEM     = mem/kern-mem.o
KERN-STDC    = kstdc/kern-stdc.o


OBJS = $(KERN-STARTUP) $(KERN-CORE) $(KERN-DEBUG) $(KERN-HAL) \
       $(KERN-LIB) $(KERN-MEM) $(KERN-STDC)

.PHONY: all
all: kernel

.PHONY: clean
clean:
	make -C core  clean
	make -C debug clean
	make -C hal   clean
	make -C kstdc clean
	make -C mem   clean
	-rm -f kernel

kernel: $(OBJS) $(scripts)/kernel.lds
	$(LINK.o) -r $^ $(LOADLIBES) $(LDLIBS) -o $@

$(KERN-STARTUP):
	make -C hal hal-startup

# The core directory must be re-maked every build because of the auto-generated
# information about the build date and time.
$(KERN-CORE): FORCE
	make -C core
	
$(KERN-DEBUG):
	make -C debug
	
$(KERN-HAL):
	make -C hal

$(KERN-LIB):
	make -C ../lib/jinue all-kernel
	
$(KERN-MEM):
	make -C mem

$(KERN-STDC):
	make -C kstdc

include $(jinue_root)/footer.mk	
