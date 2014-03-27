ENTRY (pre_kernel)
OUTPUT_ARCH (i386:x86-64)
OUTPUT_FORMAT (elf64-x86-64)

SECTIONS {

	text_prekernel : {
		*(.text_prekernel)
		*(.text_prekernel.*)
	}

	data_prekernel : {
		*(.data_prekernel)
		*(.data_prekernel.*)
	}

	.text ALIGN(0x1000) : AT(ADDR(.text)) {
		*(.text)
		*(.text.*)
	}

	.ctors ALIGN(0x1000) : AT(ADDR(.ctors)) {
		*(.ctors) 
		*(.ctors.*) 
	}

	.dtors ALIGN(0x1000) : AT(ADDR(.dtors)) {
		*(.dtors) 
		*(.dtors.*) 
	}

	.data ALIGN(0x1000) : AT(ADDR(.data)) {
		*(.data)
		*(.data.*)
	}

	.bss ALIGN(0x1000) : AT(ADDR(.bss)) {
		*(COMMON)
		*(.bss)
		*(.bss.*)
	}

	.rodata ALIGN(0x1000) : AT(ADDR(.rodata )) {
		*(.rodata)
		*(.rodata.*)
		*(.rel.rodata)
		*(.rel.rodata.*)
	}

	/DISCARD/ :
	{
		*(.comment)
		*(.eh_frame)
	}
}
