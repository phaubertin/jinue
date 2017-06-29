jinue_root = ../..
include $(jinue_root)/header.mk

OBJS   = assert.o string.o
TARGET = kern-stdc.o

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	-rm -f $(TARGET) $(OBJS)

$(TARGET): $(OBJS)
	$(LINK.o) -r $^ $(LOADLIBES) $(LDLIBS) -o $@

assert.o:      assert.c      $(includes)/kstdc/assert.h
string.o:      string.c      $(includes)/kstdc/string.h

include $(jinue_root)/footer.mk
