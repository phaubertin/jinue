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

int ipc_send(
        int                              fd,
        int                              function,
        const syscall_input_buffer_t    *buffer,
        jinue_syscall_args_t            *args) {

    thread_t *thread = get_current_thread();

    /** TODO remove this check when descriptor passing is implemented */
    if(buffer->desc_n > 0) {
        return -JINUE_ENOSYS;
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
    
    memcpy(&thread->message_buffer, buffer->user_ptr, buffer->data_size);

    /** TODO copy descriptors */

    /* return values are set by ipc_reply() (or by ipc_receive() if the call
     * fails because the message is too big for the receiver's buffer) */
    thread->message_args = args;

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
    
    /* copy reply to user space buffer */
    size_t reply_size = jinue_args_get_data_size(args);
    memcpy(buffer->user_ptr, &thread->message_buffer, reply_size);

    /** TODO copy descriptors */

    return 0;
}

int ipc_receive(
        int                              fd,
        const syscall_output_buffer_t   *buffer,
        jinue_syscall_args_t            *args) {

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
    
    size_t sender_data_size = jinue_args_get_data_size(send_thread->message_args);
    size_t sender_desc_n    = jinue_args_get_n_desc(send_thread->message_args);
    /* TODO aren't we missing padding size here? */
    size_t total_size       =
            sender_data_size + sender_desc_n * sizeof(jinue_ipc_descriptor_t);

    if(total_size > buffer->buffer_size) {
        /* message is too big for receive buffer */
        object_subref(&send_thread->header);
        thread->sender = NULL;
        
        syscall_args_set_error(send_thread->message_args, JINUE_E2BIG);
        
        /* switch back to sender thread to return from call immediately */
        thread_switch_to(send_thread, false);
                
        return -JINUE_E2BIG;
    }
    
    memcpy(
        buffer->user_ptr,
        send_thread->message_buffer,
        sender_data_size);
    
    args->arg0 = send_thread->message_args->arg0;   /* Function number */
    args->arg1 = ref->cookie;
    /* argument 2 is left intact (buffer pointer) */
    args->arg3 = send_thread->message_args->arg3;   /* Packed sized */

    return 0;
}

int ipc_reply(const syscall_input_buffer_t *buffer) {
    thread_t *thread        = get_current_thread();
    thread_t *send_thread   = thread->sender;

    if(send_thread == NULL) {
        return -JINUE_ENOMSG;
    }

    /* the reply must fit in the sender's buffer */
    size_t sender_buffer_size = jinue_args_get_buffer_size(send_thread->message_args);

    if(buffer->total_size > sender_buffer_size) {
        return -JINUE_E2BIG;
    }

    /** TODO remove this check when descriptor passing is implemented */
    if(buffer->desc_n > 0) {
        return -JINUE_ENOSYS;
    }

    memcpy(&send_thread->message_buffer, buffer->user_ptr, buffer->data_size);

    /** TODO copy descriptors */

    /** TODO set return value and error number  */
    syscall_args_set_return(send_thread->message_args, 0);
    send_thread->message_args->arg3 =
              jinue_args_pack_data_size(buffer->data_size)
            | jinue_args_pack_n_desc(buffer->desc_n);

    object_subref(&send_thread->header);
    thread->sender = NULL;
    
    /* switch back to sender thread to return from call immediately */
    thread_switch_to(send_thread, false);

    return 0;
}
