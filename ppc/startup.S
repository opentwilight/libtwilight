	.globl _start
	.globl __stack_size
_start:
	mfmsr 3
	rlwinm 3, 3, 0, 17, 15
	mtmsr 3
	isync

	lis 3, 0x8000
	addi 3, 3, 0x100

	// __bss_start and __stack_bottom are guaranteed to be aligned to the nearest 32 bytes
	lis 3, __stack_bottom@ha
	addi 3, 3, __stack_bottom@l

	lis 4, __stack_size@ha
	addi 4, 4, __stack_size@l
	lwz 4, 0(4)
	add 1, 3, 4                 // set the top of stack to the bottom plus the stack size

	lis 4, __bss_start@ha
	addi 4, 4, __bss_start@l

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
	blt- _clear_bss

	lis 4, _tw_irq_handler_template@ha
	addi 4, 4, _tw_irq_handler_template@l
	lis 5, _tw_irq_handler_template_end@ha
	addi 5, 5, _tw_irq_handler_template_end@l
	lis 6, 0x8000
	addi 6, 6, 0x100
	li 7, 0x16

_instantiate_irq_handlers_outer:
	mr 8, 6
	rlwinm 9, 6, 24, 8, 31 // shift right by 8 bits
	addis 9, 9, 0x37e0 // 0x3860 = 0x37e0 + (0x8000 >> 8)
	addi 9, 9, -1 // 0x80000100 -> 0, 0x80000200 -> 1, 0x80000300 -> 2, etc.
	stw 9, 0(8)
_copy_irq_handler:
	lwzu 9, 4(3)
	stwu 9, 4(8)
	cmpw 0, 3, 5
	blt+ _copy_irq_handler
	li 8, 0
_flush_irq_handler:
	dcbf 0, 6
	dcbst 0, 6
	sync
	icbi 0, 6
	addi 6, 6, 0x20
	cmpwi 0, 8, 8
	blt+ _flush_irq_handler
	addi 7, 7, -1
	cmpwi 0, 7, 0
	bgt+ _instantiate_irq_handlers_outer

	bl main
	b .

__stack_size:
	.long 0x30000

__irq_call_scratch:
	.long 0
	.long 0
	.long 0
	.long 0

_tw_irq_handler_template:
	li 3, 0

	lis 4, __irq_call_scratch@ha
	ori 4, 4, __irq_call_scratch@l
	stw 0, 0(4)
	stw 1, 4(4)
	stw 2, 8(4)
	mflr 5
	stw 5, 12(4)

	lis 5, _tw_irq_handler_impl@ha
	ori 5, 5, _tw_irq_handler_impl@l
	mtlr 5
	blrl

	lis 4, __irq_call_scratch@ha
	ori 4, 4, __irq_call_scratch@l
	lwz 0, 0(4)
	lwz 1, 4(4)
	lwz 2, 8(4)
	lwz 3, 12(4)
	mtlr 3

	rfi

_tw_irq_handler_template_end:
	.long 0
