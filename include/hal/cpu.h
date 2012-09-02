#ifndef _JINUE_KERNEL_CPU_H_
#define _JINUE_KERNEL_CPU_H_

typedef unsigned long msr_addr_t;


#define MSR_IA32_SYSENTER_CS	0x174

#define MSR_IA32_SYSENTER_ESP	0x175

#define MSR_IA32_SYSENTER_EIP	0x176

#define MSR_EFER				0xC0000080

#define MSR_STAR				0xC0000081


#define MSR_FLAG_STAR_SCE			(1<<0)


#define CPU_FEATURE_CPUID			(1<<0)

#define CPU_FEATURE_SYSENTER		(1<<1)

#define CPU_FEATURE_SYSCALL			(1<<2)

#define CPU_FEATURE_LOCAL_APIC		(1<<3)

#define CPU_FEATURE_PAE		        (1<<4)


#define CPU_EFLAGS_ID				(1<<21)


#define CPUID_FEATURE_FPU			(1<<0)

#define CPUID_FEATURE_PAE			(1<<6)

#define CPUID_FEATURE_APIC			(1<<9)

#define CPUID_FEATURE_SEP			(1<<11)

#define CPUID_FEATURE_HTT			(1<<28)


#define CPUID_EXT_FEATURE_SYSCALL	(1<<11)


#define CPU_VENDOR_GENERIC			0

#define CPU_VENDOR_AMD				1

#define CPU_VENDOR_INTEL			2


#define CPU_VENDOR_AMD_DW0			0x68747541	/* Auth */
#define CPU_VENDOR_AMD_DW1			0x69746e65	/* enti */
#define CPU_VENDOR_AMD_DW2			0x444d4163	/* cAMD */

#define CPU_VENDOR_INTEL_DW0		0x756e6547	/* Genu */
#define CPU_VENDOR_INTEL_DW1		0x49656e69	/* ineI */
#define CPU_VENDOR_INTEL_DW2		0x6c65746e	/* ntel */

#define CPU_CACHE_ENTRIES           8

#define CPU_CACHE_ASSOC_DIRECT      1

#define CPU_CACHE_ASSOC_FULL        (-1)


typedef enum {
    CPU_CACHE_NONE    = 0,
    CPU_CACHE_INSTR   = 1,
    CPU_CACHE_DATA    = 2,
    CPU_CACHE_UNIFIED = 3
} cpu_cache_type_t;

typedef struct {    
    cpu_cache_type_t    type;
    unsigned int        level;
    unsigned int        size;
    unsigned int        associativity;
    unsigned int        line_size;
} cpu_cache_t;


extern cpu_cache_t cpu_caches[];

extern unsigned long cpu_features;

extern unsigned long cpu_cpuid_max;

extern unsigned long cpu_cpuid_ext_max;

extern unsigned long cpu_family;

extern unsigned long cpu_model;

extern unsigned long cpu_stepping;

extern unsigned long cpu_vendor;

extern const char *cpu_vendor_name[];

extern const char *cpu_cache_type_description[];


void cpu_detect_features(void);

void cpu_detect_caches(void);

#endif
