#ifndef _JINUE_KERNEL_X86_H_
#define _JINUE_KERNEL_X86_H_

#include <hal/descriptors.h>
#include <stdint.h>

#define X86_FLAG_PG (1<<31)

typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} x86_cpuid_regs_t;

typedef uint32_t msr_addr_t;


void cli(void);

void sti(void);

void invalidate_tlb(addr_t vaddr);

void lgdt(pseudo_descriptor_t *gdt_info);

void lidt(pseudo_descriptor_t *idt_info);

void ltr(seg_selector_t sel);

uint32_t cpuid(x86_cpuid_regs_t *regs);

uint32_t get_esp(void);

uint32_t get_cr0(void);

uint32_t get_cr1(void);

uint32_t get_cr2(void);

uint32_t get_cr3(void);

uint32_t get_cr4(void);

void set_cr0(uint32_t val);

void set_cr1(uint32_t val);

void set_cr2(uint32_t val);

void set_cr3(uint32_t val);

void set_cr4(uint32_t val);

uint32_t get_eflags(void);

void set_eflags(uint32_t val);

void set_cs(uint32_t val);

void set_ds(uint32_t val);

void set_es(uint32_t val);

void set_fs(uint32_t val);

void set_gs(uint32_t val);

void set_ss(uint32_t val);

void set_data_segments(uint32_t val);

uint64_t rdmsr(msr_addr_t addr);

void wrmsr(msr_addr_t addr, uint64_t val);

uint32_t get_gs_ptr(uint32_t *ptr);

#endif

