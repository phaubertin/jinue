# Copyright (C) 2019 Philippe Aubertin.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 
# 3. Neither the name of the author nor the names of other contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# ------------------------------------------------------------------------------
# See the comment at the top of header.mk for a description of how this file fits
# in the build process and how it should be used.

.PHONY: all
all: $(targets.all)

.PHONY: clean
clean:
ifneq ($(strip $(subdirs)),)
	for i in $(subdirs); do make -C $$i clean; done
endif
ifneq ($(strip $(unclean)),)
	-rm -f $(unclean)
endif
ifneq ($(strip $(unclean_recursive)),)
	-rm -rf $(unclean_recursive)
endif

FORCE:

%.nasm: %.asm
	@echo "; * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *"	> $@
	@echo "; * This file is automatically generated by passing the assembly language"   >> $@
	@echo "; * file through the C preprocessor. Any change made here will be lost"	    >> $@
	@echo "; * on the next build."													    >> $@
	@echo "; *"																		    >> $@
	@echo "; * The file you want to edit instead is $<."							    >> $@
	@echo "; * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *"   >> $@
	@echo ""																		    >> $@
	$(CPP) $(DEPFLAGS)$(basename $@)-nasm.d $(CPPFLAGS) -x assembler-with-cpp $< >> $@

%.ld: %.lds
	@echo "/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *"	 > $@
	@echo " * This file is automatically generated by passing the linker script" 	    >> $@
	@echo " * through the C preprocessor. Any change made here will be lost on the"	    >> $@
	@echo " * next build."															    >> $@
	@echo " *"																		    >> $@
	@echo " * The file you want to edit instead is $<."								    >> $@
	@echo " * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */"   >> $@
	@echo ""																		    >> $@
	$(CPP) $(CPPFLAGS) -x c -P $< >> $@

%-nasm.o: %.nasm
	nasm -f elf $(NASMFLAGS) -o $@ $<

%.bin: %.nasm
	nasm -f bin $(NASMFLAGS) -o $@ $<

%-stripped: %
	strip $(STRIPFLAGS) -o $@ $<

.PRECIOUS: %.nasm

include $(wildcard $(depfiles))
