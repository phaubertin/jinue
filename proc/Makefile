includes = ../include
kernel   = ../kernel
lib      = ../lib
libjinue = $(lib)/jinue

OBJS  = proc.o

DEBUG   ?= yes

ifeq ($(DEBUG), yes)
	DEBUG_FLAGS = -g3
else
	DEBUG_FLAGS = -DNDEBUG
endif

CPPFLAGS	+= -I$(includes) -I$(includes)/kstdc
CFLAGS  	+= -m32 -march=i686 -O3 $(DEBUG_FLAGS) -nostdinc -ffreestanding -static -std=c99 -pedantic -Wall -Werror=implicit-function-declaration -Werror=uninitialized -Werror=return-type
LDFLAGS 	+= -Wl,-m,elf_i386 -static -nostdlib
LIBS    	 = -L$(libjinue) -ljinue -ljinue-proc

.PHONY: all
all: proc-stripped

.PHONY: clean
clean:
	-rm -f proc proc-stripped $(OBJS)

$(libjinue)/libjinue.a $(libjinue)/libjinue-proc.a: FORCE
	make -C $(libjinue)

proc: $(OBJS) $(libjinue)/libjinue.a $(libjinue)/libjinue-proc.a

proc-stripped: proc

%.o: %.asm
	nasm -f elf -o $@ $<

%-stripped: %
	strip --strip-debug -o $@ $<

FORCE:
