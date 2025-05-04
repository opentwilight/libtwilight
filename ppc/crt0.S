	.globl _start
	.globl __stack_size
_start:
	mfmsr 3
	rlwinm 3, 3, 0, 17, 15
	mtmsr 3
	isync

	// __bss_start and __stack_bottom are guaranteed to be aligned to the nearest 32 bytes
	lis 3, __stack_bottom@ha
	ori 3, 3, __stack_bottom@l

	lis 4, __stack_size@ha
	ori 4, 4, __stack_size@l
	lwz 4, 0(4)
	add 1, 3, 4                 // set the top of stack to the bottom plus the stack size

	lis 4, __bss_start@ha
	ori 4, 4, __bss_start@l

	li 24, 0
	li 25, 0
	li 26, 0
	li 27, 0
	li 28, 0
	li 29, 0
	li 30, 0
	li 31, 0

_clear_bss:
	stmw 24, 0(4)  // store 32 bytes
	dcbf 0, 4      // flush 32 bytes
	addi 4, 4, 32  // increment to next 32 byte block
	cmpw 0, 4, 3
	bne+ _clear_bss

	bl main
	b .

__stack_size:
	.long 0x30000
