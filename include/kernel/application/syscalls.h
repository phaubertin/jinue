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

#ifndef JINUE_KERNEL_APPLICATION_SYSCALLS_H
#define JINUE_KERNEL_APPLICATION_SYSCALLS_H

#include <kernel/types.h>

int close(int fd);

int create_endpoint(int fd);

int create_process(int fd);

int create_thread(int fd, int process_fd);

int destroy(int fd);

int dup(int process_fd, int src, int dest);

void exit_thread(void *exit_value);

void *get_thread_local(void);

int get_user_memory(const jinue_buffer_t *buffer);

int mclone(int src, int dest, const jinue_mclone_args_t *args);

int mint(int owner, const jinue_mint_args_t *mint_args);

int mmap(int process_fd, const jinue_mmap_args_t *args);

int puts(int loglevel, const char *str, size_t length);

void reboot(void);

int receive(int fd, jinue_message_t *message);

int reply(const jinue_message_t *message);

int send(int fd, int function, const jinue_message_t *message);

void set_thread_local(void *addr, size_t size);

int start_thread(int fd, const thread_params_t *params);

void yield_thread(void);

#endif
