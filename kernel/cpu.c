#include <cpu.h>
#include <x86.h>

unsigned long cpu_features;

unsigned long cpu_cpuid_max;

unsigned long cpu_cpuid_ext_max;

unsigned long cpu_family;

unsigned long cpu_model;

unsigned long cpu_stepping;

unsigned long cpu_vendor;

const char *cpu_vendor_name[] = {
	"Generic x86",	/* CPU_VENDOR_GENERIC */
	"AMD",			/* CPU_VENDOR_AMD */
	"Intel"			/* CPU_VENDOR_INTEL */
};


void cpu_detect_features(void) {
	unsigned long  temp;
	unsigned long  signature;
	unsigned long  flags, ext_flags;
	unsigned long  vendor_dw0, vendor_dw1, vendor_dw2;
	x86_regs_t     regs;
	
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
	}
	
	/* get extended feature flags */
	ext_flags = 0;
	
	if(cpu_features & CPU_FEATURE_CPUID && cpu_cpuid_ext_max >= 0x80000001) {
		/* extended function 1: extended feature flags */
		regs.eax = 0x80000001;
		cpu_cpuid_ext_max = cpuid(&regs);
		
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
}
