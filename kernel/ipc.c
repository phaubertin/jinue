/*
 * Copyright (C) 2019 Philippe Aubertin.
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

#include <jinue/shared/errno.h>
#include <jinue/shared/ipc.h>
#include <hal/thread.h>
#include <ipc.h>
#include <object.h>
#include <panic.h>
#include <process.h>
#include <slab.h>
#include <stddef.h>
#include <string.h>
#include <syscall.h>
#include <thread.h>


static slab_cache_t ipc_object_cache;

static ipc_t *proc_ipc = NULL;

static void ipc_object_ctor(void *buffer, size_t ignore) {
    ipc_t *ipc_object = buffer;
    
    object_header_init(&ipc_object->header, OBJECT_TYPE_IPC);
    jinue_list_init(&ipc_object->send_list);
    jinue_list_init(&ipc_object->recv_list);
}

void ipc_boot_init(void) {
    slab_cache_init(
            &ipc_object_cache,
            "ipc_object_cache",
            sizeof(ipc_t),
            0,
            ipc_object_ctor,
            NULL,
            SLAB_DEFAULTS);

    proc_ipc = slab_cache_alloc(&ipc_object_cache);

    if(proc_ipc == NULL) {
        panic("Cannot create process manager IPC object.");
    }
}

static ipc_t *ipc_object_create(int flags) {
    ipc_t *ipc = slab_cache_alloc(&ipc_object_cache);
    
    if(ipc != NULL) {
        ipc->header.flags = flags;
    }
    
    return ipc;
}

static ipc_t *ipc_get_proc_object(void) {
    return proc_ipc;
}

int ipc_create_for_current_process(int flags) {
    ipc_t *ipc;

    thread_t *thread = get_current_thread();

    int fd = process_unused_descriptor(thread->process);

    if(fd < 0) {
        return -JINUE_EAGAIN;
    }

    if(flags & JINUE_IPC_PROC) {
        ipc = ipc_get_proc_object();
    }
    else {
        int ipc_flags = IPC_FLAG_NONE;

        if(flags & JINUE_IPC_SYSTEM) {
            ipc_flags |= IPC_FLAG_SYSTEM;
        }

        ipc = ipc_object_create(ipc_flags);

        if(ipc == NULL) {
            return -JINUE_EAGAIN;
        }
    }

    object_ref_t *ref = process_get_descriptor(thread->process, fd);

    object_addref(&ipc->header);

    ref->object = &ipc->header;
    ref->flags  = OBJECT_REF_FLAG_VALID | OBJECT_REF_FLAG_OWNER;
    ref->cookie = 0;

    return fd;
}

int gather_message(thread_t *thread, const jinue_message_t *message) {
    thread->message_size = 0;

    /* TODO put an upper limit on the number of buffers in the array */

    for(int idx = 0; idx < message->send_buffers_length; ++idx) {
        jinue_buffer_t send_buffer;

        /* We are reading the buffer definition from user space so let's make
         * sure to copy the data before we check and use it to prevent it
         * from being changed by user space between steps. */
        send_buffer.addr = message->send_buffers[idx].addr;
        send_buffer.size = message->send_buffers[idx].size;

        if(! check_userspace_buffer(send_buffer.addr, send_buffer.size)) {
            return -JINUE_EINVAL;
        }

        size_t space_remaining = JINUE_SEND_MAX_SIZE - thread->message_size;

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

int ipc_send(int fd, int function, const jinue_message_t *message) {
    thread_t *thread = get_current_thread();

    /* TODO handle multiple receive buffers */
    const jinue_buffer_t *recv_buffer = &message->recv_buffers[0];
    thread->recv_buffer_size    = recv_buffer->size;
    thread->reply_errno         = 0;
    thread->message_function    = function;

    int gather_result = gather_message(thread, message);

    if(gather_result < 0) {
        return gather_result;
    }

    object_ref_t *ref = process_get_descriptor(thread->process, fd);

    if(! object_ref_is_valid(ref)) {
        return -JINUE_EBADF;
    }

    if(object_ref_is_closed(ref)) {
        return -JINUE_EIO;
    }

    object_header_t *header = ref->object;

    if(object_is_destroyed(header)) {
        ref->flags |= OBJECT_REF_FLAG_CLOSED;
        object_subref(header);
        return -JINUE_EIO;
    }

    if(header->type != OBJECT_TYPE_IPC) {
        return -JINUE_EBADF;
    }

    ipc_t *ipc = (ipc_t *)header;

    thread_t *recv_thread = jinue_node_entry(
            jinue_list_dequeue(&ipc->recv_list),
            thread_t,
            thread_list);

    if(recv_thread == NULL) {
        /* No thread is waiting to receive this message, so we must wait on the
         * sender list. */
        jinue_list_enqueue(&ipc->send_list, &thread->thread_list);
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
    memcpy(recv_buffer->addr, &thread->message_buffer, thread->message_size);

    /** TODO copy descriptors */

    return thread->message_size;
}

int ipc_receive(int fd, jinue_message_t *message) {
    thread_t *thread    = get_current_thread();
    object_ref_t *ref   = process_get_descriptor(thread->process, fd);

    if(! object_ref_is_valid(ref)) {
        return -JINUE_EBADF;
    }

    if(object_ref_is_closed(ref)) {
        return -JINUE_EIO;
    }

    if(! object_ref_is_owner(ref)) {
        return -JINUE_EPERM;
    }

    object_header_t *header = ref->object;

    if(object_is_destroyed(header)) {
        ref->flags |= OBJECT_REF_FLAG_CLOSED;
        object_subref(header);
        return -JINUE_EIO;
    }

    if(header->type != OBJECT_TYPE_IPC) {
        return -JINUE_EBADF;
    }

    ipc_t *ipc = (ipc_t *)header;
    
    thread_t *send_thread = jinue_node_entry(
        jinue_list_dequeue(&ipc->send_list),
        thread_t,
        thread_list);
    
    if(send_thread == NULL) {
        /* No thread is waiting to send a message, so we must wait on the receive
         * list. */
        jinue_list_enqueue(&ipc->recv_list, &thread->thread_list);
        thread_block();
        
        /* set by sending thread */
        send_thread = thread->sender;
    }
    else {
        object_addref(&send_thread->header);
        thread->sender = send_thread;
    }

    /* TODO handle multiple receive buffers */
    const jinue_buffer_t *recv_buffer = &message->recv_buffers[0];

    if(send_thread->message_size > recv_buffer->size) {
        /* message is too big for receive buffer */
        send_thread->reply_errno = JINUE_E2BIG;
        object_subref(&send_thread->header);
        thread->sender = NULL;
        
        /* switch back to sender thread to return from call immediately */
        thread_switch_to(send_thread, false);
                
        return -JINUE_E2BIG;
    }
    
    memcpy(
        recv_buffer->addr,
        send_thread->message_buffer,
        send_thread->message_size);
    
    message->recv_function  = send_thread->message_function;
    message->recv_cookie    = ref->cookie;
    message->reply_max_size = send_thread->recv_buffer_size;

    return 0;
}

int ipc_reply(const jinue_message_t *message) {
    thread_t *thread        = get_current_thread();
    thread_t *send_thread   = thread->sender;

    if(send_thread == NULL) {
        return -JINUE_ENOMSG;
    }

    /* TODO validate user pointer */
    /* TODO validate reply size so we don't overflow message or user space buffer */
    /* TODO handle multiple buffers */
    const jinue_buffer_t *reply_buffer = &message->send_buffers[0];

    /* the reply must fit in the sender's receive buffer */
    if(reply_buffer->size > send_thread->recv_buffer_size) {
        return -JINUE_E2BIG;
    }

    send_thread->message_size = reply_buffer->size;
    memcpy(&send_thread->message_buffer, reply_buffer->addr, reply_buffer->size);

    /** TODO copy descriptors */

    object_subref(&send_thread->header);
    thread->sender = NULL;
    
    /* switch back to sender thread to return from call immediately */
    thread_switch_to(send_thread, false);

    return 0;
}
