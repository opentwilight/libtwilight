/*
TODO: Position independent flag?
Make sure 0x8000_0000 to 0x8000_3f00 doesn't get wiped lol
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
