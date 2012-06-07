scripts  = ../scripts


KERN-CORE    = core/kern-core.o
KERN-DEBUG   = debug/kern-debug.o
KERN-HAL     = hal/kern-hal.o
KERN-MEM     = mem/kern-mem.o
KERN-STARTUP = hal/asm/startup.o

OBJS = $(KERN-STARTUP) $(KERN-CORE) $(KERN-DEBUG) $(KERN-HAL) $(KERN-MEM)

KERNEL-ASM = asm/kernel-asm.o
STARTUP    = asm/startup.o

DEBUG   ?= yes

ifeq ($(DEBUG), yes)
	DEBUG_FLAGS = -g3
else
	DEBUG_FLAGS = -DNDEBUG
endif


.PHONY: all
all: kernel

.PHONY: clean
clean:
	make -C core  clean
	make -C debug clean
	make -C hal   clean
	make -C mem   clean
	-rm -f kernel

kernel: $(OBJS) $(scripts)/kernel.lds
	$(LD) -r -T $(scripts)/kernel.lds -o $@ $(OBJS)
	

$(KERN-CORE):
	make -C core
	
$(KERN-DEBUG):
	make -C debug
	
$(KERN-HAL):
	make -C hal
	
$(KERN-MEM):
	make -C mem
	
$(KERN-STARTUP):
	make -C hal hal-startup
