/*
 * Copyright (C) 2025 Philippe Aubertin.
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

#ifndef JINUE_KERNEL_INFRASTRUCTURE_I686_ASM_MP_H
#define JINUE_KERNEL_INFRASTRUCTURE_I686_ASM_MP_H

/* Multiprocessor Specification 1.4 section 4.1 MP Floating Pointer
 * Structure */

#define MP_FLOATING_PTR_SIGNATURE   "_MP_"


#define MP_FEATURE2_IMCRP           (1 << 7)

/* Multiprocessor Specification 1.4 section 4.2 MP Configuration Table
 * Header */

#define MP_CONF_TABLE_SIGNATURE     "PCMP"

/* Multiprocessor Specification 1.4 Table 4-3 Base MP Configuration Table Entry
 * Types */

#define MP_ENTRY_TYPE_PROCESSOR     0

#define MP_ENTRY_TYPE_BUS           1

#define MP_ENTRY_TYPE_IO_APIC       2

#define MP_ENTRY_TYPE_IO_INTR       3

#define MP_ENTRY_TYPE_LOCAL_INTR    4

/* Multiprocessor Specification 1.4 Table 4-8 Bus Type String Values */

#define MP_BUS_TYPE_EISA            "EISA"

#define MP_BUS_TYPE_ISA             "ISA"

#define MP_BUS_TYPE_PCI             "PCI"

/* Multiprocessor Specification 1.4 section 4.3.3 I/O APIC Entries */

#define MP_IO_API_FLAG_EN           (1 << 0)

/* Multiprocessor Specification 1.4 Table 4-10 I/O Interrupt Entry Fields */

#define MP_PO_BUS_DEFAULT           0

#define MP_PO_ACTIVE_HIGH           1

#define MP_PO_ACTIVE_LOW            3


#define MP_EL_BUS_DEFAULT           (0 << 2)

#define MP_EL_EDGE                  (1 << 2)

#define MP_EL_LEVEL                 (2 << 2)

/* Multiprocessor Specification 1.4 Table 4-11 Interrupt Type Values */

#define MP_INTR_TYPE_INT            0

#define MP_INTR_TYPE_NMI            1

#define MP_INTR_TYPE_SMI            2

#define MP_INTR_TYPE_EXTINT         3

/* Multiprocessor Specification 1.4 Table 5-1 Default Configurations */

#define MP_NO_DEFAULT               0

#define MP_DEFAULT_ISA              1

#define MP_DEFAULT_EISA_NO_IRQ0     2

#define MP_DEFAULT_EISA             3

#define MP_DEFAULT_MCA              4

#define MP_DEFAULT_ISA_PCI_APIC     5

#define MP_DEFAULT_EISA_PIC_APIC    6

#define MP_DEFAULT_MCA_PCI_APIC     7

#endif
