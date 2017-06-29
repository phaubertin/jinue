jinue_root = ../..
include $(jinue_root)/header.mk

C_SOURCES		 = page_tables.c printk.c syscall.c vga.c
NASM_SOURCES	 = crt.asm stubs.asm

TARGET-USER		 = libjinue.a
TARGET-PROC		 = libjinue-proc.a
TARGET-KERN		 = libjinue-kernel.a

TARGETS    		 = $(TARGET-USER) $(TARGET-KERN) $(TARGET-PROC)

OBJS-USER  		 = crt.o stubs.o syscall.o
OBJS-PROC  		 = page_tables.o printk.o vga.o
OBJS-KERN  		 = page_tables.o printk.o

include $(common)

.PHONY: all-kernel
all-kernel: $(TARGET-KERN)

.PHONY: all-proc
all-proc: $(TARGET-USER) $(TARGET-PROC)

$(TARGET-USER): $(OBJS-USER)

$(TARGET-PROC): $(OBJS-PROC)

$(TARGET-KERN): $(OBJS-KERN)
