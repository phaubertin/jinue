
.PHONY: all
all: doc pdf

.PHONY: clean
clean:
	-rm -rf latex html

.PHONY: doc
doc:
	doxygen

.PHONY: pdf
pdf: doc
	make -C latex pdf
	cp latex/refman.pdf pdf/kernel-source-documentation.pdf
	
