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

#ifndef JINUE_KERNEL_INFRASTRUCTURE_ACPI_ASM_TABLES_H
#define JINUE_KERNEL_INFRASTRUCTURE_ACPI_ASM_TABLES_H


#define ACPI_FADT_SIGNATURE "FACP"

#define ACPI_HPET_SIGNATURE "HPET"

#define ACPI_MADT_SIGNATURE "APIC"

#define ACPI_RSDP_SIGNATURE "RSD PTR "

#define ACPI_RSDT_SIGNATURE "RSDT"

#define ACPI_XSDT_SIGNATURE "XSDT"

/* ACPI 6.4 section 5.2.3.2 Generic Address Structure (GAS) address space ID */

#define ACPI_GAS_ADDR_SPACE_SYSTEM_MEMORY               0x00

#define ACPI_GAS_ADDR_SPACE_SYSTEM_IO_SPACE             0x01

#define ACPI_GAS_ADDR_SPACE_PCI_CONFIG                  0x02

#define ACPI_GAS_ADDR_SPACE_EMBEDDED_CONTROLLER         0x03

#define ACPI_GAS_ADDR_SPACE_SMBUS                       0x04

#define ACPI_GAS_ADDR_SPACE_SYSTEM_CMOS                 0x05

#define ACPI_GAS_ADDR_SPACE_PCI_BAR_TARGET              0x06

#define ACPI_GAS_ADDR_SPACE_IPMI                        0x07

#define ACPI_GAS_ADDR_SPACE_GPIO                        0x08

#define ACPI_GAS_ADDR_SPACE_GENERAL_SERIAL_BUS          0x09

#define ACPI_GAS_ADDR_SPACE_PCC                         0x0a

#define ACPI_GAS_ADDR_SPACE_FUNCTIONAL_FIXED_HARDWARE   0x7f

/* ACPI 6.4 section 5.2.3.2 GAS access size */

#define ACPI_GAS_ACCESS_SIZE_UNDEFINED      0

#define ACPI_GAS_ACCESS_SIZE_BYTE_8         1

#define ACPI_GAS_ACCESS_SIZE_WORD_16        2

#define ACPI_GAS_ACCESS_SIZE_DWORD_32       3

#define ACPI_GAS_ACCESS_SIZE_QWORD_64       4

/* ACPI 6.4 section 5.2.9 values for Preferred_PM_Profile field of FADT */

#define ACPI_PM_PROFILE_UNSPECIFIED         0

#define ACPI_PM_PROFILE_DESKTOP             1

#define ACPI_PM_PROFILE_MOBILE              2

#define ACPI_PM_PROFILE_WORKSTATION         3

#define ACPI_PM_PROFILE_ENTERPRISE_SERVER   4

#define ACPI_PM_PROFILE_SOHO_SERVER         5

#define ACPI_PM_PROFILE_APPLIANCE_PC        6

#define ACPI_PM_PROFILE_PERFORMANCE_SERVER  7

#define ACPI_PM_PROFILE_TABLET              8

/* ACPI 6.4 section 5.2.9
 * Table 5.10 Fixed ACPI Description Table Fixed Feature Flags */

#define ACPI_FADT_FLAG_WBINVD                       (1 << 0)

#define ACPI_FADT_FLAG_WBINVD_FLUSH                 (1 << 1)

#define ACPI_FADT_FLAG_PROC_C1                      (1 << 2)

#define ACPI_FADT_FLAG_P_LVL2_UP                    (1 << 3)

#define ACPI_FADT_FLAG_PWR_BUTTON                   (1 << 4)

#define ACPI_FADT_FLAG_SLP_BUTTON                   (1 << 5)

#define ACPI_FADT_FLAG_FIX_RTC                      (1 << 6)

#define ACPI_FADT_FLAG_FIX_S4                       (1 << 7)

#define ACPI_FADT_FLAG_TMR_VAL_EXT                  (1 << 8)

#define ACPI_FADT_FLAG_DCK_CAP                      (1 << 9)

#define ACPI_FADT_FLAG_RESET_REG_SUP                (1 << 10)

#define ACPI_FADT_FLAG_SEALED_CASE                  (1 << 11)

#define ACPI_FADT_FLAG_HEADLESS                     (1 << 12)

#define ACPI_FADT_FLAG_CPU_SW_SLP                   (1 << 13)

#define ACPI_FADT_FLAG_PCI_EXP_WAK                  (1 << 14)

#define ACPI_FADT_FLAG_USE_PLATFORM_CLOCK           (1 << 15)

#define ACPI_FADT_FLAG_S4_RTC_STS_VALID             (1 << 16)

#define ACPI_FADT_FLAG_REMOTE_POWER_ON_CAPABLE      (1 << 17)

#define ACPI_FADT_FLAG_FORCE_APIC_CLUSTER_MODEL     (1 << 18)

#define ACPI_FADT_FLAG_FORCE_APIC_PHYS_DEST_MODE    (1 << 19)

#define ACPI_FADT_FLAG_HW_REDUCED_ACPI              (1 << 20)

#define ACPI_FADT_FLAG_LOW_POWER_S0_IDLE_CAPABLE    (1 << 21)

/* ACPI 6.4 section 5.2.9.3 IA-PC Boot Architecture Flags */

#define ACPI_IAPC_BOOT_ARCH_LEGACY_DEVICES          (1 << 0)

#define ACPI_IAPC_BOOT_ARCH_8042                    (1 << 1)

#define ACPI_IAPC_BOOT_ARCH_VGA_NOT_PRESENT         (1 << 2)

#define ACPI_IAPC_BOOT_ARCH_MSI_NOT_SUPPORTED       (1 << 3)

#define ACPI_IAPC_BOOT_ARCH_PCIE_ASPM_CONTROLS      (1 << 4)

#define ACPI_IAPC_BOOT_ARCH_CMOS_RTC_NOT_PRESENT    (1 << 5)

/* ACPI 6.4 section 5.2.9.4 ARM Architecture Boot Flags */

#define ACPI_ARM_BOOT_ARCH_PSCI_COMPLIANT           (1 << 0)

#define ACPI_ARM_BOOT_ARCH_PSCI_USE_HVC             (1 << 1)

/* ACPI 6.4 section 5.2.12 MADT flags */

#define ACPI_MADT_FLAG_PCAT_COMPAT                  (1 << 0)

/* ACPI 6.4 section 5.2.12 MADT interrupt controller structure types */

#define ACPI_MADT_ENTRY_LOCAL_APIC                  0

#define ACPI_MADT_ENTRY_IO_APIC                     1

#define ACPI_MADT_ENTRY_SOURCE_OVERRIDE             2

#define ACPI_MADT_ENTRY_NMI_SOURCE                  3

#define ACPI_MADT_ENTRY_LOCAL_APIC_NMI              4

#define ACPI_MADT_ENTRY_LOCAL_APIC_ADDR_OVERRIDE    5

#define ACPI_MADT_ENTRY_IO_SAPIC                    6

#define ACPI_MADT_ENTRY_LOCAL_SAPIC                 7

#define ACPI_MADT_ENTRY_PLATFORM_INTR_SOURCES       8

#define ACPI_MADT_ENTRY_LOCAL_X2APIC                9

#define ACPI_MADT_ENTRY_LOCAL_X2APIC_NMI            0xa

#define ACPI_MADT_ENTRY_GIC_CPU_INTERFACE_GICC      0xb

#define ACPI_MADT_ENTRY_GIC_DISTRIBUTOR_GICD        0xc

#define ACPI_MADT_ENTRY_GIC_MSI_FRAME               0xd

#define ACPI_MADT_ENTRY_GIC_REDISTRIBUTOR_GICR      0xe

#define ACPI_MADT_ENTRY_GIC_INTR_TRANSLATION_ITS    0xf

#define ACPI_MADT_ENTRY_MULTIPROCESSOR_WAKEUP       0x10

/* ACPI 6.4 section 5.2.12.2 MADT local APIC  flags */

#define ACPI_MADT_LOCAL_APIC_FLAG_ENABLED           (1 << 0)

#define ACPI_MADT_LOCAL_APIC_FLAG_ONLINE_CAPABLE    (1 << 1)

/* ACPI 6.4 section 5.2.12.5 interrupt source override flags
 *
 * These flags are identical to the equivalent ones from version 1.4 of Intel's
 * MultiProcessor Specification.
 * 
 * These flags are described in table 5.26 of the ACPI specification. They are
 * set in the flags field of the following structures in the MADT:
 *  - Interrupt Source Override Structure (type 2)
 *  - Non-Maskable Interrupt (NMI) Source Structure (type 3)
 *  - Local APIC NMI Structure (type 6)
 *  - Platform Interrupt Source Structure (type 8)
 *  - Local x2APIC NMI Structure (type 10/0xa) */

#define ACPI_MADT_MPS_INTI_FLAG_POLARITY            (1 << 0)

#define ACPI_MADT_MPS_INTI_TRIGGER_MODE_BUS         (0 << 1)

#define ACPI_MADT_MPS_INTI_TRIGGER_EDGE             (1 << 1)

#define ACPI_MADT_MPS_INTI_TRIGGER_LEVEL            (3 << 1)

#endif
