jinue_root = ../..
include $(jinue_root)/header.mk

hal_includes = $(includes)/hal

OBJS = boot.o bootmem.o cpu.o e820.o hal.o interrupt.o thread.o vga.o vm.o vm_x86.o

HAL-ASM     = asm/kern-hal-asm.o
HAL-STARTUP = asm/startup.o
TARGET      = kern-hal.o

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	make -C asm clean
	-rm -f $(TARGET) $(OBJS)
	
.PHONY: asm
asm:
	make -C asm

.PHONY: hal-startup
hal-startup : $(HAL-STARTUP)

$(HAL-STARTUP) $(HAL-ASM): asm

$(TARGET): $(OBJS) $(HAL-ASM)
	$(LINK.o) -r $^ $(LOADLIBES) $(LDLIBS) -o $@


boot.o:         boot.c          $(hal_includes)/boot.h
bootmem.o:      bootmem.c       $(hal_includes)/bootmem.h
cpu.o:          cpu.c           $(hal_includes)/cpu.h
e820.o:         e820.c          $(hal_includes)/e820.h
hal.o:          hal.c           $(hal_includes)/kernel.h
interrupt.o:    interrupt.c     $(hal_includes)/interrupt.h
thread:         thread.c        $(hal_includes)/thread.h
vga.o:          vga.c           $(hal_includes)/vga.h
vm.o:           vm.c            $(hal_includes)/vm.h
vm_x86.o:       vm_x86.c        $(hal_includes)/vm_x86.h

include $(jinue_root)/footer.mk
