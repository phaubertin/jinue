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

#ifndef _JINUE_SHARED_ASM_ERRNO_H
#define _JINUE_SHARED_ASM_ERRNO_H

/** not enough space */
#define JINUE_ENOMEM    1

/** function not supported */
#define JINUE_ENOSYS    2

/** invalid argument */
#define JINUE_EINVAL    3

/** resource unavailable, try again */
#define JINUE_EAGAIN    4

/** bad descriptor */
#define JINUE_EBADF     5

/** I/O error */
#define JINUE_EIO       6

/** operation not permitted */
#define JINUE_EPERM     7

/** argument list too long */
#define JINUE_E2BIG     8

/** no message of the desired type */
#define JINUE_ENOMSG    9

/** not supported */
#define JINUE_ENOTSUP   10

/** device or resource busy */
#define JINUE_EBUSY     11

/** no such process */
#define JINUE_ESRCH     12

/** resource deadlock would occur */
#define JINUE_EDEADLK   13

/** protocol error */
#define JINUE_EPROTO    14

#endif
