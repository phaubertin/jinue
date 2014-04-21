includes = ../../include

OBJS   = assert.o string.o
TARGET = kern-stdc.o

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

assert.o:      assert.c      $(includes)/kstdc/assert.h
string.o:      string.c      $(includes)/kstdc/string.h

%.o: %.c
	$(CC) $(CFLAGS) -c -I$(includes) -I$(includes)/kstdc -o $@ $<
