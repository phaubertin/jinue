jinue_root = ../..
include $(jinue_root)/header.mk

TARGET-USER = libjinue.a
TARGET-PROC = libjinue-proc.a
TARGET-KERN = libjinue-kernel.a

TARGETS     = $(TARGET-USER) $(TARGET-KERN) $(TARGET-PROC)

OBJS-USER   = crt.o stubs.o syscall.o
OBJS-PROC   = page_tables.o printk.o vga.o
OBJS-KERN   = page_tables.o printk.o
OBJS        = $(OBJS-USER) $(OBJS-PROC)

.PHONY: all
all: $(TARGETS)

.PHONY: all-kernel
all-kernel: $(TARGET-KERN)

.PHONY: all-proc
all-proc: $(TARGET-USER) $(TARGET-PROC)

.PHONY: clean
clean:
	-rm -f $(TARGETS) $(OBJS) $(OBJS:%.o=%.nasm)

$(TARGET-USER): $(OBJS-USER)

$(TARGET-PROC): $(OBJS-PROC)

$(TARGET-KERN): $(OBJS-KERN)

include $(jinue_root)/footer.mk
