/*
 * Copyright (C) 2019-2024 Philippe Aubertin.
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

#include <jinue/shared/asm/errno.h>
#include <jinue/shared/asm/permissions.h>
#include <jinue/shared/ipc.h>
#include <jinue/shared/vm.h>
#include <kernel/domain/entities/descriptor.h>
#include <kernel/domain/entities/endpoint.h>
#include <kernel/domain/entities/object.h>
#include <kernel/domain/entities/thread.h>
#include <kernel/domain/services/ipc.h>
#include <kernel/machine/thread.h>
#include <stddef.h>
#include <string.h>

/**
 * Check receive buffers and count receive buffer size
 *
 * @param message structure describing the receive buffers
 * @return total receive buffer size on success, negated error number on error
 *
 */
static int get_receive_buffers_size(const jinue_message_t *message) {
    size_t buffer_size = 0;

    if(message->recv_buffers_length > JINUE_MAX_BUFFERS_IN_ARRAY) {
        return -JINUE_EINVAL;
    }

    for(int idx = 0; idx < message->recv_buffers_length; ++idx) {
        const jinue_buffer_t *recv_buffer = &message->recv_buffers[idx];

        if(recv_buffer->size > JINUE_MAX_BUFFER_SIZE) {
            return -JINUE_EINVAL;
        }

        /* This is not the final check, which will happen in scatter_message()
         * while it is actually writing the message to the user space buffers.
         * We still want to make the check here though: on the sender side, we
         * don't want to send the message to the receiving thread, have it
         * process the message, and then realize we can't store the reply and,
         * on the receiver side, we don't want to find out after we dequeued a
         * sending thread.
         *
         * If things change in user space between here and when scatter_message()
         * does the write, it's fine. scatter_message() does the checks it needs
         * to protect the kernel and the application gets undefined behaviour,
         * which is fine in this context. */
        if(! check_userspace_buffer(recv_buffer->addr, recv_buffer->size)) {
            return -JINUE_EINVAL;
        }

        buffer_size += recv_buffer->size;

        /* We don't need more than this and we don't want buffer_size to
         * overflow. */
        if(buffer_size > JINUE_MAX_MESSAGE_SIZE) {
            buffer_size = JINUE_MAX_MESSAGE_SIZE;
        }
    }

    return buffer_size;
}

/**
 * Copy message or reply from user space buffer(s) to thread message buffer
 *
 * @param thread thread sending the message or reply
 * @param message structure describing the message
 * @return zero on success, negated error number on error
 *
 */
static int gather_message(thread_t *thread, const jinue_message_t *message) {
    thread->message_size = 0;

    if(message->send_buffers_length > JINUE_MAX_BUFFERS_IN_ARRAY) {
        return -JINUE_EINVAL;
    }

    for(int idx = 0; idx < message->send_buffers_length; ++idx) {
        jinue_const_buffer_t send_buffer;

        /* We are reading the buffer definition from user space so let's make
         * sure to copy the data before we check and use it to prevent it
         * from being changed by user space between steps. */
        send_buffer.addr = message->send_buffers[idx].addr;
        send_buffer.size = message->send_buffers[idx].size;

        if(! check_userspace_buffer(send_buffer.addr, send_buffer.size)) {
            return -JINUE_EINVAL;
        }

        size_t space_remaining = JINUE_MAX_MESSAGE_SIZE - thread->message_size;

        if(send_buffer.size > space_remaining) {
            return -JINUE_EINVAL;
        }

        char *write_ptr = &thread->message_buffer[thread->message_size];

        memcpy(write_ptr, send_buffer.addr, send_buffer.size);
        thread->message_size += send_buffer.size;

        /* TODO copy descriptors */
    }

    return 0;
}

/**
 * Write message or reply to user space buffer(s)
 *
 * @param thread thread receiving the message or reply
 * @param message structure describing the receive buffers
 * @return zero on success, negated error number on error
 *
 */
static int scatter_message(thread_t *thread, const jinue_message_t *message) {
    size_t read_position = 0;

    for(int idx = 0; idx < message->recv_buffers_length; ++idx) {
        size_t remaining = thread->message_size - read_position;

        if(remaining == 0) {
            break;
        }

        /* We are reading the buffer definition from user space so let's make
         * sure to copy the data before we check and use it to prevent it from
         * being changed by user space between steps. */
        jinue_buffer_t recv_buffer;
        recv_buffer.addr = message->recv_buffers[idx].addr;
        recv_buffer.size = message->recv_buffers[idx].size;

        /* We already checked this at the start of the system call but we need
         * to check it again because another application thread might have
         * changed the content of the array since. */
        if(! check_userspace_buffer(recv_buffer.addr, recv_buffer.size)) {
            return -JINUE_EINVAL;
        }

        char *read_ptr = &thread->message_buffer[read_position];

        size_t write_size = recv_buffer.size;

        if(remaining < write_size) {
            write_size = remaining;
        }

        memcpy(recv_buffer.addr, read_ptr, write_size);
        read_position += write_size;

        /* TODO copy descriptors */
    }

    return 0;
}

/**
 * Implementation of the SEND system call
 *
 * This function sends a message to an IPC endpoint so it can be received by
 * another thread, possibly in another process.
 *
 * If a receiving thread is blocked on the IPC endpoint waiting for a message,
 * then the message is processed immediately. Otherwise, the sending thread
 * blocks until a receiving thread receives the message. Threads blocked waiting
 * for a receiving thread are enqueued to a sender queue and processed in order.
 *
 * The send buffers pointed to by the message structure passed as argument
 * contain the message to be sent. The receive buffers will be used to store the
 * reply from the receiving thread.
 *
 * @param fd descriptor for the IPC endpoint
 * @param function function number of the message
 * @param message structure describing the message
 * @return message size in bytes on success, negated error number on error
 *
 */
int ipc_send(int fd, int function, const jinue_message_t *message) {
    thread_t *thread = get_current_thread();

    descriptor_t *desc;
    int status = dereference_object_descriptor(&desc, thread->process, fd);

    if(status < 0) {
        return status;
    }

    if(!descriptor_has_permissions(desc, JINUE_PERM_SEND)) {
        return -JINUE_EPERM;
    }

    ipc_endpoint_t *endpoint = get_endpoint_from_descriptor(desc);

    if(endpoint == NULL) {
        return -JINUE_EBADF;
    }

    int recv_buffer_size = get_receive_buffers_size(message);

    if(recv_buffer_size < 0) {
        return recv_buffer_size;
    }
    
    thread->recv_buffer_size    = recv_buffer_size;
    thread->reply_errno         = 0;
    thread->message_function    = function;
    thread->message_cookie      = desc->cookie;

    int gather_result = gather_message(thread, message);

    if(gather_result < 0) {
        return gather_result;
    }

    thread_t *recv_thread = jinue_node_entry(
            jinue_list_dequeue(&endpoint->recv_list),
            thread_t,
            thread_list);

    if(recv_thread == NULL) {
        /* No thread is waiting to receive this message, so we must wait on the
         * sender list. */
        jinue_list_enqueue(&endpoint->send_list, &thread->thread_list);
        thread_block();
    }
    else {
        object_addref(&thread->header);
        recv_thread->sender = thread;

        /* switch to receiver thread, which will resume inside syscall_receive() */
        thread_switch_to(recv_thread, true);
    }
    
    if(thread->reply_errno != 0) {
        return -thread->reply_errno;
    }

    /* copy reply to user space buffer */
    int scatter_result = scatter_message(thread, message);

    if(scatter_result < 0) {
        return scatter_result;
    }

    return thread->message_size;
}

/**
 * Implementation of the RECEIVE system call
 *
 * This function receives a message that another thread, probably in another
 * process, sent to a specific IPC endpoint.
 *
 * If a sending thread is blocked on the IPC endpoint waiting for a receiving
 * thread, then its message is processed immediately. Otherwise, the receiving
 * thread blocks until a sending thread attempts to send a message. Threads
 * blocked waiting to receive a message are enqueued to a receiving thread queue.
 *
 * The receive buffers pointed to by the message structure passed as argument
 * will be used to receive the message.
 *
 * @param fd descriptor for the IPC endpoint
 * @param message structure describing the receive buffers
 * @return received message size in bytes on success, negated error number on error
 *
 */
int ipc_receive(int fd, jinue_message_t *message) {
    thread_t *thread = get_current_thread();

    descriptor_t *desc;
    int status = dereference_object_descriptor(&desc, thread->process, fd);

    if(status < 0) {
        return status;
    }

    if(!descriptor_has_permissions(desc, JINUE_PERM_RECEIVE)) {
        return -JINUE_EPERM;
    }

    ipc_endpoint_t *endpoint = get_endpoint_from_descriptor(desc);

    if(endpoint == NULL) {
        return -JINUE_EBADF;
    }

    int recv_buffer_size = get_receive_buffers_size(message);

    if(recv_buffer_size < 0) {
        return recv_buffer_size;
    }

    thread_t *send_thread = jinue_node_entry(
        jinue_list_dequeue(&endpoint->send_list),
        thread_t,
        thread_list);
    
    if(send_thread == NULL) {
        /* No thread is waiting to send a message, so we must wait on the receive
         * list. */
        jinue_list_enqueue(&endpoint->recv_list, &thread->thread_list);
        thread_block();
        
        /* set by sending thread */
        send_thread = thread->sender;
    }
    else {
        object_addref(&send_thread->header);
        thread->sender = send_thread;
    }

    if(send_thread->message_size > recv_buffer_size) {
        /* message is too big for the receive buffer */
        send_thread->reply_errno = JINUE_E2BIG;
        object_subref(&send_thread->header);
        thread->sender = NULL;
        
        /* switch back to sender thread to return from call immediately */
        thread_switch_to(send_thread, false);
                
        return -JINUE_E2BIG;
    }
    
    /* copy reply to user space buffer */
    int scatter_result = scatter_message(send_thread, message);

    if(scatter_result < 0) {
        return scatter_result;
    }
    
    message->recv_function  = send_thread->message_function;
    message->recv_cookie    = send_thread->message_cookie;
    message->reply_max_size = send_thread->recv_buffer_size;

    return send_thread->message_size;
}

/**
 * Implementation of the REPLY system call
 *
 * This function is called by a receiving thread to end processing of the
 * current message and send the reply to the sending thread.
 *
 * The send buffers pointed to by the message structure passed as argument
 * contain the reply.
 *
 * @param message structure describing the message
 * @return zero on success, negated error number on error
 *
 */
int ipc_reply(const jinue_message_t *message) {
    thread_t *thread        = get_current_thread();
    thread_t *send_thread   = thread->sender;

    if(send_thread == NULL) {
        return -JINUE_ENOMSG;
    }

    int gather_result = gather_message(send_thread, message);

    if(gather_result < 0) {
        return gather_result;
    }

    /* the reply must fit in the sender's receive buffer */
    if(send_thread->message_size > send_thread->recv_buffer_size) {
        return -JINUE_E2BIG;
    }

    object_subref(&send_thread->header);
    thread->sender = NULL;
    
    /* switch back to sender thread to return from call immediately */
    thread_switch_to(send_thread, false);

    return 0;
}
