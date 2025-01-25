# Copyright (C) 2024 Philippe Aubertin.
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

jinue_root = ..
include $(jinue_root)/header.mk

test_script         = run-all-tests.gen.sh
run_script          = run-test.sh
run_log             = run-test.log

deps                = $(kernel_img) $(testapp_initrd)

tests = \
	test_acpi \
	test_boot_nopae \
	test_boot_pae \
	test_detect_qemu \
	test_mp \
	test_ipc

logs := $(foreach t,$(tests),$(t).log)

target          = $(test_script)
unclean.extra   = $(logs)

include $(common)

$(test_script): Makefile
	echo "#!/bin/bash -e" > $@
	echo "# Generated file - see Makefile" >> $@
	echo "" >> $@; \
	echo "source utils.sh" >> $@; \
	echo "" >> $@; \
	echo "fail_count=0" >> $@; \
	for t in $(tests); do \
		echo "" >> $@; \
		echo "echo =============================================================" >> $@; \
		echo "echo ==== TEST $$t" >> $@; \
		echo "echo =============================================================" >> $@; \
		echo "./$(run_script) $(kernel_img) $(testapp_initrd) ./$$t.sh $$t.log || fail_count=\$$((fail_count + 1))" >> $@; \
	done
	echo "" >> $@; \
	echo "echo =============================================================" >> $@; \
	echo "echo ==== SUMMARY" >> $@; \
	echo "echo =============================================================" >> $@; \
	echo "echo Total:    $(words $(tests))" >> $@; \
	echo "echo Success:  \$$(($(words $(tests)) - fail_count))" >> $@; \
	echo "echo Failures: \$$((fail_count))" >> $@; \
	echo '[ "$$fail_count" -gt "0" ] && report_fail' >> $@; \
	echo "report_success" >> $@; \
	chmod 755 $@

.PHONY: check
check: $(test_script) $(deps)
	./$(test_script)

.PHONY: kernel-image
kernel-image:
	make -C $(jinue_root)/kernel

.PHONY: testapp-initrd
testapp-initrd:
	make -C $(testapp)
	
$(kernel_img): kernel-image

$(testapp_initrd): testapp-initrd
