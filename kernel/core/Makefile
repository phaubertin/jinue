includes = ../../include

OBJS   = core.o elf.o syscall.o thread.o
TARGET = kern-core.o

DEBUG   ?= yes

ifeq ($(DEBUG), yes)
	DEBUG_FLAGS = -g3
else
	DEBUG_FLAGS = -DNDEBUG
endif

CFLAGS += -O3 $(DEBUG_FLAGS) -nostdinc -ffreestanding -fno-common -fno-inline -fno-omit-frame-pointer -ansi -Wall -Werror=implicit-function-declaration -Werror=uninitialized -Werror=return-type

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	-rm -f $(TARGET) $(OBJS)

$(TARGET): $(OBJS)
	$(LD) -r -o $@ $^

core.o:        core.c        $(includes)/hal/hal.h
elf.o:         elf.c         $(includes)/elf.h
syscall.o:     syscall.c     $(includes)/syscall.h
thread.o:      thread.c      $(includes)/thread.h

%.o: %.c
	$(CC) $(CFLAGS) -c -I$(includes) -I$(includes)/kstdc -o $@ $<
