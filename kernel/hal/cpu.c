#include <cpu.h>
#include <x86.h>
#include <stdint.h>

typedef struct {    
    unsigned int        descriptor;
    cpu_cache_type_t    type;
    unsigned int        level;
    unsigned int        size;
    unsigned int        associativity;
    unsigned int        line_size;
} cpu_intel_cache_descriptor_t;


cpu_cache_t cpu_caches[CPU_CACHE_ENTRIES];

unsigned int  cpu_dcache_alignment;

unsigned long cpu_features;

unsigned long cpu_cpuid_max;

unsigned long cpu_cpuid_ext_max;

unsigned long cpu_family;

unsigned long cpu_model;

unsigned long cpu_stepping;

unsigned long cpu_vendor;

const char *cpu_vendor_name[] = {
	"Generic x86",	/* CPU_VENDOR_GENERIC */
	"AMD",		/* CPU_VENDOR_AMD */
	"Intel"		/* CPU_VENDOR_INTEL */
};

const char *cpu_cache_type_description[] = {
    "(none)",
    "instruction",
    "data",
    "unified"
};

    /** TODO: 40h: "No 2nd-level cache or, if processor contains a valid
     *              2nd-level cache, no 3rd-level cache. "
     *  TODO: 49h:  "3rd-level cache: 4-MB, 16-way set associative,
     *               64-byte line size (Intel Xeon processor MP,
     *               Family 0Fh, Model 06h) "
     *  TODO: ffh:    "CPUID Leaf 2 does not report cache descriptor
     *                   information; use CPUID Leaf 4 to query cache
     *               parameters "  */
const cpu_intel_cache_descriptor_t cpu_intel_cache_descriptors[] = {
    /*  descr.  type                level   size        assoc.  line size */
    {   0x06,   CPU_CACHE_INSTR,    1,        8,             4,     32},
    {   0x08,   CPU_CACHE_INSTR,    1,       16,             4,     32},
    {   0x09,   CPU_CACHE_INSTR,    1,       32,             4,     64},
    {   0x0a,   CPU_CACHE_DATA,     1,        8,             2,     32},
    {   0x0c,   CPU_CACHE_DATA,     1,       16,             4,     32},
    {   0x0d,   CPU_CACHE_DATA,     1,       16,             4,     64},
    {   0x0e,   CPU_CACHE_DATA,     1,       24,             6,     64},
    {   0x21,   CPU_CACHE_UNIFIED,  2,      256,             8,     64},
    {   0x22,   CPU_CACHE_UNIFIED,  3,      512,             4,     64},
    {   0x23,   CPU_CACHE_UNIFIED,  3,        1 * 1024,      8,     64},
    {   0x25,   CPU_CACHE_UNIFIED,  3,        2 * 1024,      8,     64},
    {   0x29,   CPU_CACHE_UNIFIED,  3,        4 * 1024,      8,     64},
    {   0x2c,   CPU_CACHE_DATA,     1,       32,             8,     64},
    {   0x30,   CPU_CACHE_INSTR,    1,       32,             8,     64},
    {   0x41,   CPU_CACHE_UNIFIED,  2,      128,             4,     32},
    {   0x42,   CPU_CACHE_UNIFIED,  2,      256,             4,     32},
    {   0x43,   CPU_CACHE_UNIFIED,  2,      512,             4,     32},
    {   0x44,   CPU_CACHE_UNIFIED,  2,        1 * 1024,      4,     32},
    {   0x45,   CPU_CACHE_UNIFIED,  2,        2 * 1024,      4,     32},
    {   0x46,   CPU_CACHE_UNIFIED,  3,        4 * 1024,      4,     64},
    {   0x47,   CPU_CACHE_UNIFIED,  3,        8 * 1024,      8,     64},
    {   0x48,   CPU_CACHE_UNIFIED,  2,        3 * 1024,     12,     64},
    {   0x49,   CPU_CACHE_UNIFIED,  2,        4 * 1024,     16,     64},    
    {   0x4a,   CPU_CACHE_UNIFIED,  3,        6 * 1024,     12,     64},
    {   0x4b,   CPU_CACHE_UNIFIED,  3,        8 * 1024,     16,     64},
    {   0x4c,   CPU_CACHE_UNIFIED,  3,       12 * 1024,     12,     64},
    {   0x4d,   CPU_CACHE_UNIFIED,  3,       16 * 1024,     16,     64},
    {   0x4e,   CPU_CACHE_UNIFIED,  2,        6 * 1024,     24,     64},
    {   0x60,   CPU_CACHE_DATA,     1,       16,             8,     64},
    {   0x66,   CPU_CACHE_DATA,     1,        8,             4,     64},
    {   0x67,   CPU_CACHE_DATA,     1,       16,             4,     64},
    {   0x68,   CPU_CACHE_DATA,     1,       32,             4,     64},
    {   0x78,   CPU_CACHE_UNIFIED,  2,        1 * 1024,      8,     64},
    {   0x79,   CPU_CACHE_UNIFIED,  2,      128,             8,     64},
    {   0x7a,   CPU_CACHE_UNIFIED,  2,      256,             8,     64},
    {   0x7b,   CPU_CACHE_UNIFIED,  2,      512,             8,     64},
    {   0x7c,   CPU_CACHE_UNIFIED,  2,        1 * 1024,      8,     64},
    {   0x7d,   CPU_CACHE_UNIFIED,  2,        2 * 1024,      8,     64},
    {   0x7f,   CPU_CACHE_UNIFIED,  2,      512,             2,     64},
    {   0x80,   CPU_CACHE_UNIFIED,  2,      512,             8,     64},
    {   0x82,   CPU_CACHE_UNIFIED,  2,      256,             8,     32},
    {   0x83,   CPU_CACHE_UNIFIED,  2,      512,             8,     32},
    {   0x84,   CPU_CACHE_UNIFIED,  2,        1 * 1024,      8,     32},
    {   0x85,   CPU_CACHE_UNIFIED,  2,        2 * 1024,      8,     32},
    {   0x86,   CPU_CACHE_UNIFIED,  2,      512,             4,     64},
    {   0x87,   CPU_CACHE_UNIFIED,  2,        1 * 1024,      8,     64},
    {   0xd0,   CPU_CACHE_UNIFIED,  3,      512,             4,     64},
    {   0xd1,   CPU_CACHE_UNIFIED,  3,        1 * 1024,      4,     64},
    {   0xd2,   CPU_CACHE_UNIFIED,  3,        2 * 1024,      4,     64},
    {   0xd6,   CPU_CACHE_UNIFIED,  3,        1 * 1024,      8,     64},
    {   0xd7,   CPU_CACHE_UNIFIED,  3,        2 * 1024,      8,     64},
    {   0xd8,   CPU_CACHE_UNIFIED,  3,        4 * 1024,      8,     64},
    {   0xdc,   CPU_CACHE_UNIFIED,  3,       (3 * 1024)/2,  12,     64},
    {   0xdd,   CPU_CACHE_UNIFIED,  3,        3 * 1024,     12,     64},
    {   0xde,   CPU_CACHE_UNIFIED,  3,        6 * 1024,     12,     64},
    {   0xe2,   CPU_CACHE_UNIFIED,  3,        4 * 1024,     16,     64},
    {   0xe3,   CPU_CACHE_UNIFIED,  3,        2 * 1024,     16,     64},
    {   0xe4,   CPU_CACHE_UNIFIED,  3,        8 * 1024,     16,     64},
    {   0xea,   CPU_CACHE_UNIFIED,  3,       12 * 1024,     24,     64},
    {   0xeb,   CPU_CACHE_UNIFIED,  3,       18 * 1024,     24,     64},
    {   0xec,   CPU_CACHE_UNIFIED,  3,       24 * 1024,     24,     64},
    
    {   0x00,   0, 0, 0, 0, 0}
};
    

void cpu_detect_features(void) {
	uint32_t         temp;
	uint32_t         signature;
	uint32_t         flags, ext_flags;
	uint32_t         vendor_dw0, vendor_dw1, vendor_dw2;
	x86_regs_t       regs;
	
	cpu_features = 0;
	
	/* The CPUID instruction is available if we can change the value of eflags
	 * bit 21 (ID) */
	temp  = get_eflags();
	temp ^= CPU_EFLAGS_ID;
	set_eflags(temp);
	
	if(temp == get_eflags()) {
	    cpu_features |= CPU_FEATURE_CPUID;
	}
	
	/* get CPU vendor ID string */	
	if(cpu_features & CPU_FEATURE_CPUID) {
		/* function 0: vendor ID string, max value of eax when calling CPUID */
		regs.eax = 0;
		
		/* call CPUID instruction */
		cpu_cpuid_max = cpuid(&regs);
		vendor_dw0 = regs.ebx;
		vendor_dw1 = regs.edx;
		vendor_dw2 = regs.ecx;
		
		/* identify vendor */
		if(    vendor_dw0 == CPU_VENDOR_AMD_DW0 
			&& vendor_dw1 == CPU_VENDOR_AMD_DW1
			&& vendor_dw2 == CPU_VENDOR_AMD_DW2) {
				
			cpu_vendor = CPU_VENDOR_AMD;
		}
		else if (vendor_dw0 == CPU_VENDOR_INTEL_DW0
			&&   vendor_dw1 == CPU_VENDOR_INTEL_DW1
			&&   vendor_dw2 == CPU_VENDOR_INTEL_DW2) {
				
			cpu_vendor = CPU_VENDOR_INTEL;
		}
		else {
			cpu_vendor = CPU_VENDOR_GENERIC;
		}
		
		/* extended function 0: max value of eax when calling CPUID (extended function) */
		regs.eax = 0x80000000;
		cpu_cpuid_ext_max = cpuid(&regs);
	}
	
	/* get processor signature (family/model/stepping) and feature flags */
	cpu_family         = 0;
	cpu_model          = 0;
	cpu_stepping       = 0;
	flags              = 0;
	
	/* default value */
	cpu_dcache_alignment = 32;
	
	if(cpu_features & CPU_FEATURE_CPUID && cpu_cpuid_max >= 1) {
		/* function 1: processor signature and feature flags */
		regs.eax = 1;
		
		/* call CPUID instruction */
		signature = cpuid(&regs);
		
		/* set processor signature */
		cpu_stepping   = signature       & 0xf;
		cpu_model      = (signature>>4)  & 0xf;
		cpu_family     = (signature>>8)  & 0xf;
		
		/* feature flags */
		flags = regs.edx;
		
		/* cache alignment */
		if(flags & CPUID_FEATURE_CLFLUSH) {
		    cpu_dcache_alignment = ((regs.ebx >> 8) & 0xff) * 8;
		}
	}
	
	/* get extended feature flags */
	ext_flags = 0;
	
	if(cpu_features & CPU_FEATURE_CPUID && cpu_cpuid_ext_max >= 0x80000001) {
		/* extended function 1: extended feature flags */
		regs.eax = 0x80000001;
		(void)cpuid(&regs);
		
		/* extended feature flags */
		ext_flags = regs.edx;
	}
	
	/* support for sysenter/sysexit */
	if(flags & CPUID_FEATURE_SEP) {
		if(cpu_vendor == CPU_VENDOR_AMD) {
			cpu_features |= CPU_FEATURE_SYSENTER;
		}
		else if(cpu_vendor == CPU_VENDOR_INTEL) {
			if(cpu_family == 6 && cpu_model < 3 && cpu_stepping < 3) {
				/* not supported */
			}
			else {
				cpu_features |= CPU_FEATURE_SYSENTER;
			}
		}
	}
	
	/* support for syscall/sysret */
	if(cpu_vendor == CPU_VENDOR_AMD) {
		if(ext_flags & CPUID_EXT_FEATURE_SYSCALL) {
			cpu_features |= CPU_FEATURE_SYSCALL;
		}
	}
	
	/* support for local APIC */
	if(cpu_vendor == CPU_VENDOR_AMD || cpu_vendor == CPU_VENDOR_INTEL) {
		if(flags & CPUID_FEATURE_APIC) {
			cpu_features |= CPU_FEATURE_LOCAL_APIC;
		}
	}
	
	/* support for physical address extension (PAE) */
	if(cpu_vendor == CPU_VENDOR_AMD || cpu_vendor == CPU_VENDOR_INTEL) {
		if(flags & CPUID_FEATURE_PAE) {
			cpu_features |= CPU_FEATURE_PAE;
		}
	}
	
	cpu_detect_caches();
}

void cpu_detect_caches(void) {
    unsigned int   idx;
    unsigned int   loop_idx;
    unsigned int   descr;
    unsigned int   size;
    unsigned int   val;
    unsigned int   associativity;
    x86_regs_t     regs;
    
    /* Initialize all cache entries with type CPU_CACHE_NONE. The entry
     * following the last valid one is expected to have this type 
     * (list terminator). */
    for(idx = 0; idx < CPU_CACHE_ENTRIES; ++idx) {
	cpu_caches[idx].type = CPU_CACHE_NONE;
    }
    
    if(cpu_vendor == CPU_VENDOR_INTEL && cpu_cpuid_max >= 2) {
	/** TODO: not yet implemented */
    }
    else if(cpu_vendor == CPU_VENDOR_AMD) {
	idx = 0;
	
	if(cpu_cpuid_ext_max >= 0x80000005) {
	    regs.eax = 0x80000005;
	    (void)cpuid(&regs);
	    
	    /* first loop iteration: L1 instruction cache */
	    descr = regs.edx;
	    
	    for(loop_idx = 0; loop_idx < 2; ++loop_idx) {
		size          =  descr >> 24;
		associativity = (descr >> 16) & 0xff;
		if(size > 0 && associativity > 0) {
		    cpu_caches[idx].size      = size;
		    cpu_caches[idx].level     = 1;
		    
		    if(loop_idx == 0) {
			cpu_caches[idx].type  = CPU_CACHE_INSTR;
		    }
		    else {
			cpu_caches[idx].type  = CPU_CACHE_DATA;
		    }
		    
		    cpu_caches[idx].line_size = descr & 0xff;
		    
		    if(associativity < 255) {
			cpu_caches[idx].associativity = associativity;
		    }
		    else {
			/* 255 (0xff) means fully associative */
			cpu_caches[idx].associativity = CPU_CACHE_ASSOC_FULL;
		    }
		    
		    ++idx;
		}
		
		/* second loop iteration: L1 instruction cache */
		descr = regs.ecx;
	    }
	    
	}
	if(cpu_cpuid_ext_max >= 0x80000006) {
	    regs.eax = 0x80000006;
	    (void)cpuid(&regs);
	    
	    /* first loop iteration: L2 cache */
	    descr = regs.ecx;
	    
	    for(loop_idx = 0; loop_idx < 2; ++loop_idx) {
		size =  descr >> 16;
		
		if(loop_idx > 0) {
		    /* edx (L3 cache) has a different format than ecx (L2).
		     * It is stored in bits 31..18 (instead of 31..16) and is in 512kB units.
		     * 
		     * " [31:18] L3Size: L3 cache size. Specifies the L3 cache size is within the following range:
		     *           (L3Size[31:18] * 512KB) <= L3 cache size < ((L3Size[31:18]+1) * 512KB). "
		     *  AMD CPUID Specification (Publication # 25481) revision 2.34 pp.25.  */
		    size = (size >> 2) * 512;
		}
				
		val  = (descr >> 12) & 0xf;
		
		switch(val) {
		case 1:
		case 2:
		case 4:
		    associativity = val;
		    break;
		case 0x6:
		    associativity = 8;
		    break;
		case 0x8:
		    associativity = 16;
		    break;
		case 0xa:
		    associativity = 32;
		    break;
		case 0xb:
		    associativity = 48;
		    break;
		case 0xc:
		    associativity = 64;
		    break;
		case 0xd:
		    associativity = 96;
		    break;
		case 0xe:
		    associativity = 128;
		    break;
		case 0xf:
		    associativity = CPU_CACHE_ASSOC_FULL;
		    break;
		default:
		    associativity = 0;
		}
		
		if(size > 0 && associativity > 0) {
		    cpu_caches[idx].size      = size;
		    cpu_caches[idx].type      = CPU_CACHE_UNIFIED;
		    
		    if(loop_idx == 0) {
			cpu_caches[idx].level  = 2;
		    }
		    else {
			cpu_caches[idx].level  = 3;
		    }
		    
		    cpu_caches[idx].line_size     = descr & 0xff;
		    cpu_caches[idx].associativity = associativity;
		    
		    ++idx;
		}
		
		/* first loop iteration: L3 cache */
		descr = regs.edx;
	    }
	}
    }
}
