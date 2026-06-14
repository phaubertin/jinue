#!/bin/bash
# Copyright (C) 2024-2026 Philippe Aubertin.
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

CPU=core2duo
CMDLINE="DEBUG_DUMP_SYSCALL_IMPLEMENTATION=1"

run

check_kernel_start

# If check_no_panic, check_no_error would also fail, but check_no_panic provides
# more relevant context in the log.
check_no_panic

check_no_error

echo "* Check CPU vendor, family and model for Core 2 Duo"
grep -F "Vendor: Intel family: 6 model: 15 stepping:" $LOG || fail

echo "* Check PAE was enabled"
grep -F "Physical Address Extension (PAE) is enabled." $LOG || fail

echo "* Check NX was enabled"
grep -F "No eXecute (NX) protection is enabled." $LOG || fail

check_no_warning

check_loader_start

check_testapp_start

check_reboot
