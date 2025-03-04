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

CMDLINE="RUN_TEST_IPC=1"

run

echo "* Check the threading and IPC test ran"
grep -F "Running threading and IPC test" $LOG || fail

check_no_error

check_no_warning

echo "* Check the main thread tried to receive on the send-only descriptor"
grep -F "Attempting to call jinue_receive() on the send-only descriptor." $LOG || fail

echo "* Check errno was set to JINUE_EPERM"
RESULT=`grep -F -A 1 "Attempting to call jinue_receive() on the send-only descriptor." $LOG`
echo "$RESULT" | grep -F 'operation not permitted' || fail

echo "* Check client thread started and got the right argument"
grep -F "Client thread is starting with argument: 0xb01dface" $LOG || fail

echo "* Check main thread received message from client thread"
grep -F "Main thread received message" $LOG || fail

MESSAGE=`grep -F -A 5 "Main thread received message:" $LOG`

echo "* Check message data"
echo "$MESSAGE" | grep -E 'data:[ ]+"Hello World!"' || fail

echo "* Check message size"
echo "$MESSAGE" | grep -E 'size:[ ]+13$' || fail

echo "* Check function number"
echo "$MESSAGE" | grep -E 'function:[ ]+4138\b' || fail

echo "* Check cookie"
echo "$MESSAGE" | grep -E 'cookie:[ ]+0xca11ab1e$' || fail

echo "* Check client thread received reply from main thread"
grep -F "Client thread got reply from main thread" $LOG || fail

REPLY=`grep -F -A 2 "Client thread got reply from main thread:" $LOG`

echo "* Check reply data"
echo "$REPLY" | grep -E 'data:[ ]+"Hi, Main Thread!"' || fail

echo "* Check message size"
echo "$REPLY" | grep -E 'size:[ ]+17$' || fail

echo "* Check client thread attempted to send the message a second time"
grep -F "Client thread is re-sending message." $LOG || fail

echo "* Check main thread closed the receive descriptor while the client was send blocked"
RESULT=`grep -F -A 1 "Client thread is re-sending message." $LOG`
echo "$RESULT" | grep -F 'Closing receiver descriptor.' || fail

echo "* Check closing the receive descriptor caused JINUE_EIO in the send-blocked thread"
RESULT=`grep -F -A 1 "Closing receiver descriptor." $LOG`
echo "$RESULT" | grep -F 'I/O error' || fail

echo "* Check client thread exited cleanly"
grep -F "Client thread is exiting." $LOG || fail

echo "* Check main thread joined the client thread and retrieved its exit value"
grep -F "Client thread exit value is 0xdeadbeef." $LOG || fail

echo "* Check the main thread initiated the reboot"
grep -F "Main thread is running." $LOG || fail
grep -F "Rebooting." $LOG || fail
