/*
 * Copyright (C) 2024 Philippe Aubertin.
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
#include <jinue/utils.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "../descriptors.h"
#include "meminfo.h"
#include "server.h"

int reply_error(int error_number) {
    int status = jinue_reply_error(error_number, &errno);

    if(status < 0) {
        jinue_error("jinue_reply_error() failed: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int receive_message(jinue_message_t *message) {
    message->recv_buffers           = NULL;
    message->recv_buffers_length    = 0;

    int status = jinue_receive(JINUE_DESC_LOADER_ENDPOINT, message, &errno);

    if(status < 0) {
        jinue_error("jinue_receive() failed: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int run_server(void) {
    while(true) {
        jinue_message_t message;

        int status = receive_message(&message);

        if(status != EXIT_SUCCESS) {
            return status;
        }

        switch(message.recv_function) {
            case JINUE_MSG_GET_MEMINFO:
                status = get_meminfo(&message);

                if(status != EXIT_SUCCESS) {
                    return status;
                }
                break;
            case JINUE_MSG_EXIT:
                /* Exit without sending back a response. This will cause the call to fail with
                 * JINUE_EIO on the sender's side, but only once this process has exited. */
                return EXIT_SUCCESS;
            default:
                status = reply_error(JINUE_ENOSYS);

                if(status != EXIT_SUCCESS) {
                    return status;
                }
        }
    }
}
