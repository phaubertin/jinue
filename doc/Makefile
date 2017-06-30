jinue_root = ..
include $(jinue_root)/header.mk

unclean          = latex html
targets.phony	 = pdf

include $(common)

.PHONY: doc
doc:
	doxygen

.PHONY: pdf
pdf: doc
	make -C latex pdf
	cp latex/refman.pdf pdf/kernel-source-documentation.pdf
	
