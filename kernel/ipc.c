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

#include <jinue-common/errno.h>
#include <jinue-common/ipc.h>
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

ipc_t *ipc_object_create(int flags) {
    ipc_t *ipc = slab_cache_alloc(&ipc_object_cache);
    
    if(ipc != NULL) {
        ipc->header.flags = flags;
    }
    
    return ipc;
}

ipc_t *ipc_get_proc_object(void) {
    return proc_ipc;
}

void ipc_send(jinue_syscall_args_t *args) {
    thread_t *thread = get_current_thread();

    message_info_t *message_info = &thread->message_info;

    message_info->function      = args->arg0;
    message_info->buffer_size   = jinue_args_get_buffer_size(args);
    message_info->data_size     = jinue_args_get_data_size(args);
    message_info->desc_n        = jinue_args_get_n_desc(args);
    message_info->total_size    =
            message_info->data_size +
            message_info->desc_n * sizeof(jinue_ipc_descriptor_t);

    if(message_info->buffer_size > JINUE_SEND_MAX_SIZE) {
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }

    if(message_info->total_size > message_info->buffer_size) {
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }
    
    if(message_info->desc_n > JINUE_SEND_MAX_N_DESC) {
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }

    /** TODO remove this check when descriptor passing is implemented */
    if(message_info->desc_n > 0) {
        syscall_args_set_error(args, JINUE_ENOSYS);
        return;
    }

    int fd = (int)args->arg1;

    object_ref_t *ref = process_get_descriptor(thread->process, fd);

    if(! object_ref_is_valid(ref)) {
        syscall_args_set_error(args, JINUE_EBADF);
        return;
    }

    if(object_ref_is_closed(ref)) {
        syscall_args_set_error(args, JINUE_EIO);
        return;
    }

    message_info->cookie = ref->cookie;

    object_header_t *header = ref->object;

    if(object_is_destroyed(header)) {
        ref->flags |= OBJECT_REF_FLAG_CLOSED;
        object_subref(header);

        syscall_args_set_error(args, JINUE_EIO);
        return;
    }

    if(header->type != OBJECT_TYPE_IPC) {
        syscall_args_set_error(args, JINUE_EBADF);
        return;
    }

    ipc_t *ipc = (ipc_t *)header;

    char *user_ptr = (char *)args->arg2;
    
    if(! user_buffer_check(user_ptr, message_info->buffer_size)) {
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }

    memcpy(&thread->message_buffer, user_ptr, message_info->data_size);

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

        thread_yield_from(
                thread,
                true,       /* make thread block */
                false);     /* don't destroy */
    }
    else {
        object_addref(&thread->header);
        recv_thread->sender = thread;

        /* switch to receiver thread, which will resume inside syscall_receive() */
        thread_switch(
                thread,
                recv_thread,
                true,       /* block sender thread */
                false);     /* don't destroy sender */
    }
    
    /* copy reply to user space buffer */
    memcpy(user_ptr, &thread->message_buffer, message_info->data_size);

    /** TODO copy descriptors */
}

void ipc_receive(jinue_syscall_args_t *args) {
    thread_t *thread = get_current_thread();

    int fd = (int)args->arg1;

    object_ref_t *ref = process_get_descriptor(thread->process, fd);

    if(! object_ref_is_valid(ref)) {
        syscall_args_set_error(args, JINUE_EBADF);
        return;
    }

    if(object_ref_is_closed(ref)) {
        syscall_args_set_error(args, JINUE_EIO);
        return;
    }

    if(! object_ref_is_owner(ref)) {
        syscall_args_set_error(args, JINUE_EPERM);
        return;
    }

    object_header_t *header = ref->object;

    if(object_is_destroyed(header)) {
        ref->flags |= OBJECT_REF_FLAG_CLOSED;
        object_subref(header);

        syscall_args_set_error(args, JINUE_EIO);
        return;
    }

    if(header->type != OBJECT_TYPE_IPC) {
        syscall_args_set_error(args, JINUE_EBADF);
        return;
    }

    ipc_t *ipc = (ipc_t *)header;
    
    char *user_ptr = (char *)args->arg2;
    size_t buffer_size = jinue_args_get_buffer_size(args);
    
    if(! user_buffer_check(user_ptr, buffer_size)) {
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }
    
    thread_t *send_thread = jinue_node_entry(
        jinue_list_dequeue(&ipc->send_list),
        thread_t,
        thread_list);
    
    if(send_thread == NULL) {
        /* No thread is waiting to send a message, so we must wait on the receive
         * list. */
        jinue_list_enqueue(&ipc->recv_list, &thread->thread_list);

        thread_yield_from(
                thread,
                true,       /* make thread block */
                false);     /* don't destroy */
        
        /* set by sending thread */
        send_thread = thread->sender;
    }
    else {
        object_addref(&send_thread->header);
        thread->sender = send_thread;
    }
    
    if(send_thread->message_info.total_size > buffer_size) {
        /* message is too big for receive buffer */
        object_subref(&send_thread->header);
        thread->sender = NULL;
        
        syscall_args_set_error(send_thread->message_args, JINUE_E2BIG);
        syscall_args_set_error(args, JINUE_E2BIG);
        
        /* switch back to sender thread to return from call immediately */
        thread_switch(
                thread,
                send_thread,
                false,      /* don't block (put this thread back in ready queue) */
                false);     /* don't destroy */
                
        return;
    }
    
    memcpy(
        user_ptr,
        send_thread->message_buffer,
        send_thread->message_info.data_size);
    
    args->arg0 = send_thread->message_args->arg0;
    args->arg1 = ref->cookie;
    /* argument 2 is left intact (buffer pointer) */
    args->arg3 = send_thread->message_args->arg3;
}

void ipc_reply(jinue_syscall_args_t *args) {
    thread_t *thread        = get_current_thread();
    thread_t *send_thread   = thread->sender;

    if(send_thread == NULL) {
        /** TODO is there a better error number for this situation? */
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }

    size_t buffer_size   = jinue_args_get_buffer_size(args);
    size_t data_size     = jinue_args_get_data_size(args);
    size_t desc_n        = jinue_args_get_n_desc(args);
    size_t total_size    =
            data_size +
            desc_n * sizeof(jinue_ipc_descriptor_t);

    if(buffer_size > JINUE_SEND_MAX_SIZE) {
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }
    
    if(total_size > buffer_size) {
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }

    if(desc_n > JINUE_SEND_MAX_N_DESC) {
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }

    /* the reply must fit in the sender's buffer */
    if(total_size > send_thread->message_info.buffer_size) {
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }

    /** TODO remove this check when descriptor passing is implemented */
    if(desc_n > 0) {
        syscall_args_set_error(args, JINUE_ENOSYS);
        return;
    }

    const char *user_ptr = (const char *)args->arg2;

    if(! user_buffer_check(user_ptr, buffer_size)) {
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }

    memcpy(&send_thread->message_buffer, user_ptr, data_size);

    /** TODO copy descriptors */

    /** TODO set return value and error number  */
    syscall_args_set_return(send_thread->message_args, 0);
    send_thread->message_args->arg3 =
            args->arg3 & ~(JINUE_SEND_SIZE_MASK << JINUE_SEND_BUFFER_SIZE_OFFSET);

    send_thread->message_info.data_size = data_size;
    send_thread->message_info.desc_n    = desc_n;

    object_subref(&send_thread->header);
    thread->sender = NULL;

    syscall_args_set_return(args, 0);
    
    /* switch back to sender thread to return from call immediately */
    thread_switch(
            thread,
            send_thread,
            false,      /* don't block (put this thread back in ready queue) */
            false);     /* don't destroy */
}
