includes = ../../include

OBJS   = pfalloc.o slab.o vm_alloc.o
TARGET = kern-mem.o

DEBUG   ?= yes

ifeq ($(DEBUG), yes)
	DEBUG_FLAGS = -g3
else
	DEBUG_FLAGS = -DNDEBUG
endif

CFLAGS += -m32 -march=i686 -O3 $(DEBUG_FLAGS) -nostdinc -ffreestanding -fno-common -fno-inline -fno-omit-frame-pointer -ansi -Wall -Werror=implicit-function-declaration -Werror=uninitialized -Werror=return-type

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	-rm -f $(TARGET) $(OBJS)

$(TARGET): $(OBJS)
	$(LD) -r -m elf_i386 -o $@ $^

pfalloc.o:     pfalloc.c     $(includes)/pfalloc.h
slab.o:        slab.c        $(includes)/slab.h
vm_alloc.o:    vm_alloc.c    $(includes)/vm_alloc.h

%.o: %.c
	$(CC) $(CFLAGS) -c -I$(includes) -I$(includes)/kstdc -o $@ $<
