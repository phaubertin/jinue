#ifndef _JINUE_KERNEL_X86_H_
#define _JINUE_KERNEL_X86_H_

#define X86_FLAG_PG (1<<31)

void cli(void);

void sti(void);

void invalidate_tlb(addr_t vaddr);

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

#endif

