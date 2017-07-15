jinue_root = ../..
include $(jinue_root)/header.mk

sources.c        = ipc.c page_tables.c printk.c syscall.c console.c
sources.nasm     = crt.asm stubs.asm

target.user      = libjinue.a
target.proc      = libjinue-proc.a
target.kernel    = libjinue-kernel.a

targets          = $(target.user) $(target.kernel) $(target.proc)

objects.user     = crt.o ipc.o stubs.o syscall.o
objects.proc     = page_tables.o printk.o console.o
objects.kernel   = page_tables.o printk.o

include $(common)

.PHONY: all-kernel
all-kernel: $(target.kernel)

.PHONY: all-proc
all-proc: $(target.user) $(target.proc)

$(target.user): $(objects.user)

$(target.proc): $(objects.proc)

$(target.kernel): $(objects.kernel)

%.a:
	ar rcs $@ $^
