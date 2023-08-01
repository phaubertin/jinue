/*
 * Copyright (C) 2023 Philippe Aubertin.
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

#ifndef _JINUE_LIBC_TAR_H
#define _JINUE_LIBC_TAR_H

/** ustar header magic*/
#define TMAGIC "ustar"


/** regular file */
#define REGTYPE     '0'

/** regular file */
#define AREGTYPE    '\0'

/** link */
#define LNKTYPE     '1'

/** symbolic link */
#define SYMTYPE     '2'

/** character special */
#define CHRTYPE     '3'

/** block special */
#define BLKTYPE     '4'

/** directory */
#define DIRTYPE     '5'

/** FIFO special */
#define FIFOTYPE    '6'

/** contiguous file - treated as a regular file */
#define CONTTYPE    '7'


/** set UID on execution */
#define TSUID       04000

/** set GID on execution */
#define TSGID       02000

/** on directories, restricted deletion flag */
#define TSVTX       01000

/** read by owner */
#define TUREAD      00400

/** write by owner special */
#define TUWRITE     00200

/** execute/search by owner */
#define TUEXEC      00100

/** read by group */
#define TGREAD      00040

/** write by group */
#define TGWRITE     00020

/** execute/search by group */
#define TGEXEC      00010

/** read by other */
#define TOREAD      00004

/** write by other */
#define TOWRITE     00002

/** execute/search by other */
#define TOEXEC      00001

#endif
