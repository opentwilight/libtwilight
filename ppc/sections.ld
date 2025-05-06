/*
TODO: Position independent flag?
Make sure 0x8000_0000 to 0x8000_3f00 doesn't get wiped lol
*/

PHDRS {
	app PT_LOAD;
}

SECTIONS {
	. = 0x80003F00;

	.text : { *(.text) *(.text.*) } :app
	.data : { *(.data) *(.data.*) }
	.sdata : { *(.sdata) *(.sdata.*) }
	.rodata : { *(.rodata) *(.rodata.*) }

	. = ALIGN(0x20);
	__bss_start = .;
	.sbss : { *(.sbss) *(.sbss.*) }
	.bss : { *(.bss) *(.bss.*) }
	__bss_end = .;

	. = ALIGN(0x40);
	__toc_region = .;
}
