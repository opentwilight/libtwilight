/*
MEMORY {
	. = 0x80003F00;
	.stack : {
		. = . + 0x30000;
		__stack_top = .;
		__heap_mem1_start = .;
	}
}
*/

SECTIONS {
	. = 0x80003F00;
	.text : { *(.text) *(.text.*) }
	.data : { *(.data) *(.data.*) }
	.sdata : { *(.sdata) *(.sdata.*) }
	.rodata : { *(.rodata) *(.rodata.*) }

	. = ALIGN(0x20);
	__bss_start = .;
	.sbss : { *(.sbss) *(.sbss.*) }
	.bss : { *(.bss) *(.bss.*) }
	__bss_end = .;

	. = ALIGN(0x40);
	__stack_bottom = .;
}
