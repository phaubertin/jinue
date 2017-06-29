FORCE:

%.nasm: %.asm
	$(CPP) $(CPPFLAGS) -x assembler-with-cpp -o $@ $<

%.o: %.nasm
	nasm $(NASMFLAGS) -o $@ $<

%.a:
	ar rcs $@ $^

%-stripped: %
	strip --strip-debug -o $@ $<
