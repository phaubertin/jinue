OUTPUT_FORMAT("binary")
TARGET("binary")
INPUT("bin/boot" "bin/setup" "bin/kernel-bin")

SECTIONS {
	boot : {
		bin/boot
	}
	
	setup : {
		bin/setup	
	}
	
	kernel : {
		bin/kernel-bin
	}	
}
