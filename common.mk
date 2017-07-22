.PHONY: all
all: $(targets.all)

.PHONY: clean
clean:
ifneq ($(strip $(subdirs)),)
	for i in $(subdirs); do make -C $$i clean; done
endif
	-rm -f $(unclean)
ifneq ($(strip $(unclean_recursive)),)
	-rm -rf $(unclean_recursive)
endif

FORCE:

%.nasm: %.asm
	$(CPP) $(DEPFLAGS) $(CPPFLAGS) -x assembler-with-cpp -o $@ $<

%.o: %.nasm
	nasm -f elf $(NASMFLAGS) -o $@ $<

%.bin: %.nasm
	nasm -f bin $(NASMFLAGS) -o $@ $<

%-stripped: %
	strip $(STRIPFLAGS) -o $@ $<

.PRECIOUS: %.nasm

include $(wildcard $(patsubst %,%.d,$(basename $(objects))))
