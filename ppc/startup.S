	.globl _start
	.globl __toc_size
	.globl __stack_size
	.globl _finish_startup
_start:
	// disable interrupts
	mfmsr 3
	rlwinm 6, 3, 0, 17, 15 // clear the EE bit
	mtmsr 6 // we'll leave the non-EE MSR in r6 for now
	isync

	// initialize block address translations (BATs)
	// involves setting HID0 (SPR 1008) to 0x00110c64 to disable icache and dcache, then restoring them afterward with 0x0011cc64
	// TODO

	// initialize L2 cache
	li 3, 0x30 // disable everything except BATs
	mtmsr 3
	sync

	// 1017 = L2 Cache Control Register
	mfspr 3, 1017
	rlwinm 3, 3, 0, 1, 31 // clear the MSB (L2E)
	mtspr 1017, 3
	sync
	mfspr 3, 1017
	oris 3, 3, 0x0020 // enable L2 global invalidation
	mtspr 1017, 3

_wait_for_l2cr_1:
	mfspr 3, 1017
	andi. 3, 3, 1
	bne+ _wait_for_l2cr_1

	mfspr 3, 1017
	rlwinm 3, 3, 0, 11, 9 // clear the invalidate bit (0x0020)
	mtspr 1017, 3

_wait_for_l2cr_2:
	mfspr 3, 1017
	andi. 3, 3, 1
	bne+ _wait_for_l2cr_2

	oris 3, 3, 0x8000
	rlwinm 3, 3, 0, 11, 9
	mtspr 1017, 3
	sync

	// restore normal MSR
	mtmsr 6
	sync

_after_l2_setup:

	// setup stack and toc
	// __bss_start and __toc_region are guaranteed to be aligned to the nearest 32 bytes
	lis 3, __toc_region@ha
	addi 2, 3, __toc_region@l
	mr 13, 2

	lis 5, __toc_size@ha
	lwz 4, __toc_size@l(5)
	add 3, 2, 4                 // set the bottom of stack to the end of the toc table

	lis 5, __stack_size@ha
	lwz 4, __stack_size@l(5)
	add 1, 3, 4                 // set the top of stack to the bottom plus the stack size

	li 24, 0
	li 25, 0
	li 26, 0
	li 27, 0
	li 28, 0
	li 29, 0
	li 30, 0
	li 31, 0

	lis 4, __bss_start@ha
	addi 4, 4, __bss_start@l

_clear_bss:
	stmw 24, 0(4)  // store 32 bytes
	dcbf 0, 4      // flush 32 bytes
	addi 4, 4, 32  // increment to next 32 byte block
	cmpw 0, 4, 3
	blt- _clear_bss

	lis 24, _tw_interrupt_handler_template@ha
	addi 24, 24, _tw_interrupt_handler_template@l
	lis 25, _tw_interrupt_handler_template_end@ha
	addi 25, 25, _tw_interrupt_handler_template_end@l
	lis 26, 0x8000
	addi 26, 26, 0x100
	li 27, 0x16

_instantiate_irq_handlers_outer:
	mr 3, 24
	mr 8, 26
	rlwinm 9, 26, 24, 8, 31 // shift right by 8 bits
	addis 9, 9, 0x37e0 // 0x3860 = 0x37e0 + (0x8000 >> 8)
	addic 9, 9, -1 // 0x80000100 -> 0, 0x80000200 -> 1, 0x80000300 -> 2, etc.
	stw 9, 0(8)
_copy_irq_handler:
	lwzu 9, 4(3)
	stwu 9, 4(8)
	cmpw 0, 3, 25
	blt+ _copy_irq_handler
	mr 3, 26
	li 4, 0x100
	bl TW_FlushMemory
	mr 26, 3
	addic. 27, 27, -1
	bge+ _instantiate_irq_handlers_outer

	sync

	bl TW_EnableInterrupts
	bl main

	// TODO: do thread cleanup here (joining, etc)
	// TODO: return to loader or power off
	b .

__toc_size:
	.long 0x8000
__stack_size:
	.long 0x30000

.globl __tw_external_interrupt_handler_table
__tw_external_interrupt_handler_table:
	.long 0
	.long 0
	.long 0
	.long 0
	.long 0
	.long 0
	.long 0
	.long 0
	.long 0
	.long 0
	.long 0
	.long 0
	.long 0
	.long 0
	.long 0
	.long 0

__interrupt_call_scratch:
	.long 0
	.long 0
	.long 0
	.long 0

_tw_interrupt_handler_template:
	li 3, 0 // not copied, just here for readability

	lis 4, __interrupt_call_scratch@ha
	ori 4, 4, __interrupt_call_scratch@l
	stw 0, 0(4)
	stw 1, 4(4)
	stw 2, 8(4)
	mflr 5
	stw 5, 12(4)

	lis 5, _tw_interrupt_handler_impl@ha
	ori 5, 5, _tw_interrupt_handler_impl@l
	mtlr 5
	blrl

	lis 4, __interrupt_call_scratch@ha
	ori 4, 4, __interrupt_call_scratch@l
	lwz 0, 0(4)
	lwz 1, 4(4)
	lwz 2, 8(4)
	lwz 3, 12(4)
	mtlr 3

	rfi

_tw_interrupt_handler_template_end:
	.long 0
