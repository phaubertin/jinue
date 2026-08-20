/*
 * Copyright (C) 2019-2026 Philippe Aubertin.
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the author nor the names of other contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <jinue/jinue.h>
#include <jinue/loader.h>
#include <jinue/utils.h>
#include <srv/system.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "tests/tests.h"
#include "utils.h"

int do_exit() {
    jinue_message_t message;
    message.send_buffers        = NULL;
    message.send_buffers_length = 0;
    message.recv_buffers        = NULL;
    message.recv_buffers_length = 0;

    /* TODO define a constant for the endpoint descriptor */
    intptr_t retval = jinue_send(JINUE_DESC_LOADER_ENDPOINT, SYS_MSG_EXIT, &message, &errno, NULL);

    if(retval < 0) {
        jinue_error("error: jinue_send() failed on exit: %s.", strerror(errno));
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    /* Say hello. */
    jinue_info("Jinue test app (%s) started.", argv[0]);

    run_abcd_test();
    run_aes_test();
    run_cancel_thread_test();
    run_cancel_thread_async_test();
    run_exit_thread_test();
    run_ipc_test();
    run_scroll_test();
    run_signal_test();
    run_sse_test();

    return do_exit();
}
