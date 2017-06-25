includes = ../../include
scripts = ../../scripts

OBJS   = core.o elf.o syscall.o
TARGET = kern-core.o

DEBUG   ?= yes

ifeq ($(DEBUG), yes)
	DEBUG_FLAGS = -g3
else
	DEBUG_FLAGS = -DNDEBUG
endif

CFLAGS += -m32 -march=i686 -O3 $(DEBUG_FLAGS) -nostdinc -ffreestanding -fno-common -fno-inline -fno-omit-frame-pointer -std=c99 -pedantic -Wall -Werror=implicit-function-declaration -Werror=uninitialized -Werror=return-type

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	-rm -f $(TARGET) $(OBJS)

$(TARGET): $(OBJS)
	$(LD) -r -m elf_i386 -o $@ $^

core.o:        core.c        $(includes)/hal/hal.h build-info.h
elf.o:         elf.c         $(includes)/elf.h
syscall.o:     syscall.c     $(includes)/syscall.h

build-info.h: FORCE
	$(scripts)/gen_build_info_h.sh > $@

%.o: %.c
	$(CC) $(CFLAGS) -c -I$(includes) -I$(includes)/kstdc -o $@ $<

FORCE:
