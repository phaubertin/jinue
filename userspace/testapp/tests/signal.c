/*
 * Copyright (C) 2026 Philippe Aubertin.
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
#include <internals.h>
#include <signal.h>
#include "../utils.h"
#include "signal.h"

sig_atomic_t signal_1_flag;

sig_atomic_t signal_32_flag;

sig_atomic_t signal_33_flag;

sig_atomic_t signal_64_flag;

sig_atomic_t nested_signal_1_flag;

void signal_1_handler(int sig) {
    jinue_info("In signal 1 handler");

    if(sig != 1) {
        jinue_error("Expected signal 1, got %d", sig);
        return;
    }

    signal_1_flag += 1;
}

void signal_32_sa_handler(int sig, siginfo_t *info, void *context) {
    jinue_info("In signal 32 handler");

    if(sig != 32) {
        jinue_error("Expected signal 32, got %d (sig)", sig);
        return;
    }

    if(info->si_signo != 32) {
        jinue_error("Expected signal 32, got %d (siginfo)", sig);
        return;
    }

    signal_32_flag += 1;
}

void signal_33_handler(int sig) {
    jinue_info("In signal 33 handler");

    if(sig != 33) {
        jinue_error("Expected signal 33, got %d", sig);
        return;
    }

    signal_33_flag += 1;
}

void signal_64_handler(int sig) {
    jinue_info("In signal 64 handler");

    if(sig != 64) {
        jinue_error("Expected signal 64, got %d", sig);
        return;
    }

    signal_64_flag += 1;
}

void nested_signal_1_handler(int sig) {
    jinue_info("In signal 1 handler");

    if(sig != 1) {
        jinue_error("Expected signal 1, got %d", sig);
        return;
    }

    signal_1_flag += 1;

    if(signal_1_flag == 1) {
        raise(1);

        /* Calling any system call will deliver any pending signal, including a
         * nested signal 1 if it isn't blocked. */
        jinue_yield_thread();

        if(signal_1_flag > 1) {
            nested_signal_1_flag = 1;
        }
    }
}

#define CHECK_TRUE(b) if(!(b)) {return false;}

#define CHECK_FALSE(b) if(b) {return false;}

#define CHECK_ZERO(d) if((d) != 0) {return false;}

static bool setup(void) {
    signal_1_flag           = 0;
    signal_32_flag          = 0;
    signal_33_flag          = 0;
    signal_64_flag          = 0;
    nested_signal_1_flag    = 0;

    sigset_t set;
    sigemptyset(&set);

    CHECK_ZERO(sigprocmask(SIG_SETMASK, &set, NULL));

    struct sigaction act;
    act.sa_flags = 0;
    act.sa_handler = signal_1_handler;
    sigemptyset(&act.sa_mask);

    CHECK_ZERO(sigaction(1, &act, NULL));

    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = signal_32_sa_handler;

    CHECK_ZERO(sigaction(32, &act, NULL));

    act.sa_flags = 0;
    act.sa_handler = signal_33_handler;

    CHECK_ZERO(sigaction(33, &act, NULL));

    act.sa_flags = 0;
    act.sa_handler = signal_64_handler;

    CHECK_ZERO(sigaction(64, &act, NULL));

    return true;
}

static bool test_raise_1(void) {
    CHECK_ZERO(raise(1));

    CHECK_TRUE(signal_1_flag == 1);
    CHECK_TRUE(signal_32_flag == 0);
    CHECK_TRUE(signal_33_flag == 0);
    CHECK_TRUE(signal_64_flag == 0);

    return true;
}

static bool test_raise_32(void) {
    CHECK_ZERO(raise(32));

    CHECK_TRUE(signal_1_flag == 0);
    CHECK_TRUE(signal_32_flag == 1);
    CHECK_TRUE(signal_33_flag == 0);
    CHECK_TRUE(signal_64_flag == 0);

    return true;
}

static bool test_raise_33(void) {
    CHECK_ZERO(raise(33));

    CHECK_TRUE(signal_1_flag == 0);
    CHECK_TRUE(signal_32_flag == 0);
    CHECK_TRUE(signal_33_flag == 1);
    CHECK_TRUE(signal_64_flag == 0);

    return true;
}

static bool test_raise_64(void) {
    CHECK_ZERO(raise(64));

    CHECK_TRUE(signal_1_flag == 0);
    CHECK_TRUE(signal_32_flag == 0);
    CHECK_TRUE(signal_33_flag == 0);
    CHECK_TRUE(signal_64_flag == 1);

    return true;
}

static bool test_raise_65(void) {
    int status = raise(65);

    CHECK_TRUE(status == -1);
    CHECK_TRUE(errno == EINVAL);

    CHECK_TRUE(signal_1_flag == 0);
    CHECK_TRUE(signal_32_flag == 0);
    CHECK_TRUE(signal_33_flag == 0);
    CHECK_TRUE(signal_64_flag == 0);

    return true;
}

static bool test_sigemptyset_sigaddset(void) {
    sigset_t set;

    sigemptyset(&set);

    CHECK_FALSE(sigismember(&set, 1));
    CHECK_FALSE(sigismember(&set, 10));
    CHECK_FALSE(sigismember(&set, 32));
    CHECK_FALSE(sigismember(&set, 33));
    CHECK_FALSE(sigismember(&set, 42));
    CHECK_FALSE(sigismember(&set, 64));

    CHECK_ZERO(sigaddset(&set, 10));
    CHECK_ZERO(sigaddset(&set, 42));

    CHECK_FALSE(sigismember(&set, 1));
    CHECK_TRUE(sigismember(&set, 10));
    CHECK_FALSE(sigismember(&set, 32));
    CHECK_FALSE(sigismember(&set, 33))
    CHECK_TRUE(sigismember(&set, 42));
    CHECK_FALSE(sigismember(&set, 64));

    return true;
}

static bool test_sigfillset_sigdelset(void) {
    sigset_t set;

    sigfillset(&set);

    CHECK_TRUE(sigismember(&set, 1));
    CHECK_TRUE(sigismember(&set, 10));
    CHECK_TRUE(sigismember(&set, 32));
    CHECK_TRUE(sigismember(&set, 33));
    CHECK_TRUE(sigismember(&set, 42));
    CHECK_TRUE(sigismember(&set, 64));

    CHECK_ZERO(sigdelset(&set, 10));
    CHECK_ZERO(sigdelset(&set, 42));

    CHECK_TRUE(sigismember(&set, 1));
    CHECK_FALSE(sigismember(&set, 10));
    CHECK_TRUE(sigismember(&set, 32));
    CHECK_TRUE(sigismember(&set, 33));
    CHECK_FALSE(sigismember(&set, 42));
    CHECK_TRUE(sigismember(&set, 64));

    return true;
}

static bool test_sigprocmask_block_signal(void) {
    sigset_t set;

    jinue_info("Retrieving current signal mask");

    CHECK_ZERO(sigprocmask(SIG_UNBLOCK, NULL, &set));

    /* Cleared by setup() */
    CHECK_FALSE(sigismember(&set, 1));

    jinue_info("Blocking signal 1");

    CHECK_ZERO(sigaddset(&set, 1));
    CHECK_ZERO(sigprocmask(SIG_BLOCK, &set, NULL));

    jinue_info("raise(1)");

    /* Should leave the signal pending since it is blocked. */
    CHECK_ZERO(raise(1));

    jinue_info("Signal 1 should remain pending");

    CHECK_TRUE(signal_1_flag == 0);

    jinue_yield_thread();

    /* still blocked */
    CHECK_TRUE(signal_1_flag == 0);

    jinue_info("Unblocking signal 1");

    CHECK_ZERO(sigprocmask(SIG_UNBLOCK, &set, NULL));

    jinue_info("Signal 1 should have been delivered");

    /* signal delivered once unblocked */
    CHECK_TRUE(signal_1_flag == 1);

    return true;
}

static bool test_pthread_sigmask_block_signal(void) {
    sigset_t set;

    jinue_info("Retrieving current signal mask");

    CHECK_ZERO(pthread_sigmask(SIG_UNBLOCK, NULL, &set));

    /* Cleared by setup() */
    CHECK_FALSE(sigismember(&set, 64));

    jinue_info("Blocking signal 64");

    CHECK_ZERO(sigaddset(&set, 64));
    CHECK_ZERO(pthread_sigmask(SIG_BLOCK, &set, NULL));

    jinue_info("raise(64)");

    /* Should leave the signal pending since it is blocked. */
    CHECK_ZERO(raise(64));

    jinue_info("Signal 64 should remain pending");

    CHECK_TRUE(signal_64_flag == 0);

    jinue_yield_thread();

    /* still blocked */
    CHECK_TRUE(signal_64_flag == 0);

    jinue_info("Unblocking signal 64");

    CHECK_ZERO(pthread_sigmask(SIG_UNBLOCK, &set, NULL));

    jinue_info("Signal 64 should have been delivered");

    /* signal delivered once unblocked */
    CHECK_TRUE(signal_64_flag == 1);

    return true;
}

static bool test_jinue_get_set_signal_mask_how_none_invalid_set(void) {
    jinue_sigset_t set;

    jinue_sigemptyset(&set);
    CHECK_ZERO(sigaddset(&set, 12));
    CHECK_ZERO(sigaddset(&set, 44));

    CHECK_ZERO(jinue_get_set_signal_mask(JINUE_SIG_SETMASK, &set, NULL, NULL));

    jinue_sigemptyset(&set);

    CHECK_FALSE(sigismember(&set, 1));
    CHECK_FALSE(sigismember(&set, 12));
    CHECK_FALSE(sigismember(&set, 44));
    CHECK_FALSE(sigismember(&set, 64));

    CHECK_ZERO(jinue_get_set_signal_mask(
        JINUE_SIG_NONE,
        (const jinue_sigset_t *)(NULL + 40),
        &set,
        NULL)
    );

    CHECK_FALSE(sigismember(&set, 1));
    CHECK_TRUE(sigismember(&set, 12));
    CHECK_TRUE(sigismember(&set, 44));
    CHECK_FALSE(sigismember(&set, 64));

    return true;
}

static bool test_signal_process(void) {
    CHECK_ZERO(jinue_signal_process(-1, 1, NULL));

    CHECK_TRUE(signal_1_flag == 1);

    return true;
}

#define MSG_SYNCHRONIZE JINUE_SYS_USER_BASE

static int endpoint;

static void *thread_func(void *arg) {
    jinue_info("Thread started");

    jinue_message_t message;
    message.send_buffers_length = 0;
    message.recv_buffers_length = 0;

    jinue_info("Thread: wait 1");

    intptr_t ret = jinue_send(endpoint, MSG_SYNCHRONIZE, &message, &errno, NULL);

    if(ret < 0) {
        jinue_error("error: jinue_send() failed: %s.", strerror(errno));
    }

    jinue_info("Thread: wait 2");

    ret = jinue_send(endpoint, MSG_SYNCHRONIZE, &message, &errno, NULL);

    if(ret < 0) {
        jinue_error("error: jinue_send() failed: %s.", strerror(errno));
    }

    jinue_info("Thread: unblocking signal 1");

    sigset_t set;
    CHECK_ZERO(sigemptyset(&set));
    CHECK_ZERO(sigaddset(&set, 1));
    CHECK_ZERO(sigprocmask(SIG_UNBLOCK, &set, NULL));

    return NULL;
}

static bool test_signal_other_thread(void) {
    jinue_info("Creating IPC endpoint");

    endpoint = libc_allocate_descriptor();
    CHECK_ZERO(jinue_create_endpoint(endpoint, NULL));

    jinue_info("Blocking signal 1");

    sigset_t set;
    CHECK_ZERO(sigemptyset(&set));
    CHECK_ZERO(sigaddset(&set, 1));
    CHECK_ZERO(sigprocmask(SIG_BLOCK, &set, NULL));

    jinue_info("Starting thread");

    pthread_t thread;
    CHECK_ZERO(start_thread(&thread, thread_func, NULL));

    jinue_yield_thread();

    jinue_info("Unblocking signal 1 in main thread");

    CHECK_ZERO(sigprocmask(SIG_UNBLOCK, &set, NULL));

    jinue_info("Sending signal 1");

    CHECK_ZERO(jinue_signal_thread(get_thread_descriptor(thread), 1, JINUE_SIG_FLAG_NONE, NULL));

    jinue_info("Confirming signal is was not delivered to the main thread");

    CHECK_TRUE(signal_1_flag == 0);

    jinue_info("Letting thread proceed from 'wait 1'");

    jinue_message_t message;
    message.recv_buffers_length = 0;
    jinue_message_t reply;
    reply.send_buffers_length = 0;

    CHECK_ZERO(jinue_receive(endpoint, &message, NULL));
    CHECK_ZERO(jinue_reply(&reply, NULL));

    jinue_yield_thread();

    jinue_info("Confirming thread inherited blocked signal from main thread");

    CHECK_TRUE(signal_1_flag == 0);

    jinue_info("Letting thread proceed from 'wait 2'");

    message.recv_buffers_length = 0;
    reply.send_buffers_length = 0;

    CHECK_ZERO(jinue_receive(endpoint, &message, NULL));
    CHECK_ZERO(jinue_reply(&reply, NULL));

    jinue_yield_thread();

    jinue_info("Waiting for thread to terminate");

    CHECK_ZERO(pthread_join(thread, NULL));

    jinue_info("Confirming signal 1 was delivered to thread");

    CHECK_TRUE(signal_1_flag == 1);

    return true;
}

static bool test_nested_signal(void) {
    struct sigaction act;
    act.sa_flags = 0;
    act.sa_handler = nested_signal_1_handler;
    sigemptyset(&act.sa_mask);

    CHECK_ZERO(sigaction(1, &act, NULL));

    raise(1);

    jinue_info("Checking signal 1 was blocked while within the signal handler");

    CHECK_TRUE(nested_signal_1_flag == 0);

    jinue_info("Checking signal 1 was unblocked while returning from the signal handler");

    jinue_yield_thread();

    CHECK_TRUE(signal_1_flag == 2);

    return true;
}

typedef bool (*test_t)(void);

bool run_test(test_t test, const char *name) {
    jinue_info("Running test: %s", name);

    bool pass = setup();

    if(!pass) {
        jinue_info("setup() failed");
    }
    else {
        pass = test();
    }

    jinue_info("Test %s: %s", name, pass ? "PASS" : "FAIL");
    jinue_info("---");

    return pass;
}

void run_signal_test(void) {
    if(! bool_getenv("RUN_TEST_SIGNAL")) {
        return;
    }

    jinue_info("Running signal test...");

    bool pass = true;

    pass &= run_test(test_raise_1, "raise(1)");
    pass &= run_test(test_raise_32, "raise(32)");
    pass &= run_test(test_raise_33, "raise(33)");
    pass &= run_test(test_raise_64, "raise(64)");
    pass &= run_test(test_raise_65, "raise(65) EINVAL");
    pass &= run_test(test_sigemptyset_sigaddset, "sigemptyset() and sigaddset()");
    pass &= run_test(test_sigfillset_sigdelset, "sigfillset() and sigdelset()");
    pass &= run_test(test_sigprocmask_block_signal, "sigprocmask() block signal");
    pass &= run_test(test_pthread_sigmask_block_signal, "pthread_sigmask() block signal");
    pass &= run_test(test_jinue_get_set_signal_mask_how_none_invalid_set, "jinue_get_set_signal_mask() how=JINUE_SIG_NONE invalid set");
    pass &= run_test(test_signal_process, "jinue_signal_process()");
    pass &= run_test(test_signal_other_thread, "signal other thread");
    pass &= run_test(test_nested_signal, "nested signal");

    jinue_info("Signal test result: %s", pass ? "PASS" : "FAIL");
}
