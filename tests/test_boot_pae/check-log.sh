#!/bin/bash
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

fail () {
    echo "*** [ FAIL ] ***" >&2
    exit 1
}

usage () {
    echo "USAGE: $(basename $0) log_file" >&2
    exit 1
}

[[ $# -ne 1 ]] && usage

echo "* Check log file exists"
[[ -f $1 ]] || fail

echo "* Check kernel started"
grep -F "Jinue microkernel started." $1 || fail

echo "* Check kernel did not panic"
grep -F -A 20 "KERNEL PANIC" $1 && fail

echo "* Check no error occurred"
grep -E "^error:" $1 && fail

echo "* Check PAE was enabled"
grep -F "Enabling Physical Address Extension (PAE)" $1 || fail

echo "* Check no warning was reported"
grep -E "^warning:" $1 && fail

echo "* Check user space loader started"
grep -F "Jinue user space loader (jinue-userspace-loader) started." $1 || fail

echo "* Check test application started"
grep -F "Jinue test app (/sbin/init) started." $1 || fail

echo "* Check the test application initiated the reboot"
grep -F "Rebooting." $1 || fail

echo "[ PASS ]"
