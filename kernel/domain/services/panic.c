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

#include <kernel/domain/services/logging.h>
#include <kernel/domain/config.h>
#include <kernel/machine/debug.h>
#include <kernel/machine/halt.h>
#include <stdbool.h>

void panic(const char *message) {
    static int enter_count = 0;

    /* When things go seriously wrong, things that panic() does can themselves
     * create a further kernel panic, for example by triggering a hardware
     * exception. The enter_count static variable keeps count of the number of
     * times panic() is entered recursively and is used to prevent an infinite
     * recursive loop. */
    ++enter_count;

    switch(enter_count) {
    case 1:
    case 2:
        /* The first two times panic() is entered, a panic message is displayed
         * along with a full call stack dump. */
        emergency("KERNEL PANIC%s: %s", (enter_count == 1) ? "" : " (recursive)", message);
        machine_dump_call_stack();
        break;
    case 3:
        /* The third time, a "recursive count exceeded" message is displayed. We
         * try to limit the number of actions we take to limit the chances of a
         * further panic. */
        emergency("KERNEL PANIC (recursive count exceeded)");
        break;
    default:
        /* The fourth time, we do nothing but halt the CPU. */
        break;
    }

    const config_t *config = get_config();

    if(config->on_panic == CONFIG_ON_PANIC_REBOOT) {
        machine_reboot();
    }
    
    machine_halt();
}
