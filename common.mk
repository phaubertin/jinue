.PHONY: all
all: $(ALL_TARGETS)

.PHONY: clean
clean:
ifneq ($(strip $(subdirs)),)
	for i in $(subdirs); do make -C $$i clean; done
endif
	-rm -f $(UNCLEAN)

FORCE:

%.nasm: %.asm
	$(CPP) $(CPPFLAGS) -x assembler-with-cpp -o $@ $<

%.o: %.nasm
	nasm $(NASMFLAGS) -o $@ $<

%.a:
	ar rcs $@ $^

%-stripped: %
	strip --strip-debug -o $@ $<