#ifndef _JINUE_KERNEL_X86_H_
#define _JINUE_KERNEL_X86_H_

#include <cpu.h>
#include <descriptors.h>

#define X86_FLAG_PG (1<<31)

typedef struct {
	unsigned long eax;
	unsigned long ebx;
	unsigned long ecx;
	unsigned long edx;
} x86_regs_t;

void cli(void);

void sti(void);

void invalidate_tlb(addr_t vaddr);

void lgdt(gdt_info_t *gdt_info);

void lidt(idt_info_t *idt_info);

void ltr(seg_selector_t sel);

unsigned long cpuid(x86_regs_t *regs);

unsigned long get_cr0(void);

unsigned long get_cr1(void);

unsigned long get_cr2(void);

unsigned long get_cr3(void);

unsigned long get_cr4(void);

void set_cr0(unsigned long val);

void set_cr0x(unsigned long val);

void set_cr1(unsigned long val);

void set_cr2(unsigned long val);

void set_cr3(unsigned long val);

void set_cr4(unsigned long val);

unsigned long get_eflags(void);

void set_eflags(unsigned long val);

void set_cs(unsigned long val);

void set_ds(unsigned long val);

void set_es(unsigned long val);

void set_fs(unsigned long val);

void set_gs(unsigned long val);

void set_ss(unsigned long val);

void set_data_segments(unsigned long val);

unsigned long long rdmsr(msr_addr_t addr);

void wrmsr(msr_addr_t addr, unsigned long long val);

#endif

