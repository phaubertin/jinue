#!/bin/bash -e
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

# Default test setup parameters
CPU=core2duo
MEM=128
CMDLINE=""

usage () {
    echo "USAGE: $(basename $0) kernel_image initrd test_script log_file" >&2
    exit 1
}

[[ $# -ne 4 ]] && usage

KERNEL_IMAGE=$1
INITRD=$2
TEST_SCRIPT=$3
LOG=$4

run () {
    BASE_CMDLINE="on_panic=reboot serial_enable=yes serial_dev=/dev/ttyS0 DEBUG_DO_REBOOT=1"

    qemu-system-i386 \
        -cpu ${CPU} \
        -m ${MEM} \
        -no-reboot \
        -kernel "${KERNEL_IMAGE}" \
        -initrd "${INITRD}" \
        -append "${BASE_CMDLINE} ${CMDLINE}" \
        -serial stdio \
        -display none \
        -smp 1 \
        -usb \
        -vga std | tee $LOG
    
    echo =============================================================
}

# This ensures the test reliably fails if "run" is never called.
[ -f $LOG ] && rm -v $LOG

source utils.sh
( source $TEST_SCRIPT ) || report_fail

report_success
