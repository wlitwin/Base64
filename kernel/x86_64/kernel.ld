ENTRY (pre_kernel)
OUTPUT_ARCH (i386:x86-64)
OUTPUT_FORMAT (elf64-x86-64)

SECTIONS {

	. = 0x100000;
	__KERNEL_STACK_END = .;

	. = 0x200000;
	/* Stack operations decrement first */
	__KERNEL_STACK_START = .;
	__KERNEL_START = .;

	text_prekernel ALIGN(0x1000) : AT(0x200000) {
		prekernel_start = .;
		*(.text_prekernel)
		*(.text_prekernel.*)
	}

	data_prekernel ALIGN(0x1000) : {
		*(.data_prekernel)
		*(.data_prekernel.*)
	}

	.text ALIGN(0x1000) : AT(ADDR(.text)) {
		KERNEL_START = .;
		code = .; _code = .; __code = .;
		*(.text)
		*(.text.*)
	}

	.ctors ALIGN(0x1000) : AT(ADDR(.ctors)) {
		start_of_ctors = .; 
		*(.ctors) 
		*(.ctors.*) 
		end_of_ctors = .;
	}

	.dtors ALIGN(0x1000) : AT(ADDR(.dtors)) {
		start_of_dtors = .; 
		*(.dtors) 
		*(.dtors.*) 
		end_of_dtors = .;
	}

	.data ALIGN(0x1000) : AT(ADDR(.data)) {
		data_start = .; __data_start = .;
		data = .; _data = .; __data = .;
		*(.data)
		*(.data.*)
	}

	.bss ALIGN(0x1000) : AT(ADDR(.bss)) {
		sbss = .;
		bss = .; _bss = .; __bss = .;
		*(COMMON)
		*(.bss)
		*(.bss.*)
		ebss = .;
	}

	.rodata ALIGN(0x1000) : AT(ADDR(.rodata )) {
		*(.rodata)
		*(.rodata.*)
		*(.rel.rodata)
		*(.rel.rodata.*)
	}

	end = .; _end = .; __end = .;
	__KERNEL_END = .;

	/DISCARD/ :
	{
		*(.comment)
		*(.eh_frame)
	}
}
