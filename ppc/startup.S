	.globl _start
	.globl __toc_size
	.globl __stack_size
	.globl _finish_startup

_init_bats:
	// initialize block address translations (BATs)
	lis 7, 0x0011
	ori 7, 7, 0x0c64
	mtspr 1008, 7 // set HID0 (SPR 1008) to 0x00110c64 to disable icache and dcache
	isync

	lis 3, 0x8000
	mtsr 0, 3
	mtsr 1, 3
	mtsr 2, 3
	mtsr 3, 3
	mtsr 4, 3
	mtsr 5, 3
	mtsr 6, 3
	mtsr 7, 3
	mtsr 8, 3
	mtsr 9, 3
	mtsr 10, 3
	mtsr 11, 3
	mtsr 12, 3
	mtsr 13, 3
	mtsr 14, 3
	mtsr 15, 3
	isync

	li 4, 0
	mtspr 528, 4
	mtspr 529, 4
	mtspr 530, 4
	mtspr 531, 4
	mtspr 532, 4
	mtspr 533, 4
	mtspr 534, 4
	mtspr 535, 4
	mtspr 536, 4
	mtspr 537, 4
	mtspr 538, 4
	mtspr 539, 4
	mtspr 540, 4
	mtspr 541, 4
	mtspr 542, 4
	mtspr 543, 4

#if TW_WII
	isync
	lis 3, 0x8200
	mtspr 1011, 3 // set HID4 (is it SPR 1011?) to 0x82 << 24 to enable HID4 access and BATs 5-8
	isync

	mtspr 544, 4
	mtspr 545, 4
	mtspr 546, 4
	mtspr 547, 4
	mtspr 548, 4
	mtspr 549, 4
	mtspr 550, 4
	mtspr 551, 4
	mtspr 552, 4
	mtspr 553, 4
	mtspr 554, 4
	mtspr 555, 4
	mtspr 556, 4
	mtspr 557, 4
	mtspr 558, 4
	mtspr 559, 4
	isync

	// MEM2
	// Set BAT4 U,L to 0x9000_1fff, 0x1000_0002 for instruction and data
	lis 3, 0x9000
	ori 3, 3, 0x1fff
	lis 4, 0x1000
	ori 4, 4, 2
	mtspr 544, 3
	mtspr 545, 4
	mtspr 552, 3
	mtspr 553, 4
	isync

	// Wii-specific MMIO
	// Set BAT5 U,L to 0xd000_1fff, 0x1000_002a for data only
	oris 3, 3, 0x4000
	ori 4, 4, 0x28
	mtspr 554, 3
	mtspr 555, 4
#endif

	isync

	// MEM1
	// Set BAT0 U,L to 0x8000_1fff, 0x0000_0002 for instruction and data
	lis 3, 0x8000
	ori 3, 3, 0x1fff
	li 4, 2
	mtspr 528, 3
	mtspr 529, 4
	mtspr 536, 3
	mtspr 537, 4
	isync

	// MMIO
	// Set BAT1 U,L to 0xc000_1fff, 0x0000_002a for data only
	oris 3, 3, 0x4000
	ori 4, 4, 0x28
	mtspr 538, 3
	mtspr 539, 4
	isync

	// re-enable icache and dcache
	mfspr 3, 1008 // (HID0)
	ori 3, 3, 0xc000
	mtspr 1008, 3
	isync

	mfmsr 3
	ori 3, 3, 0x30 // enable BATs for instructions and data
	mtsrr1 3
	mflr 3 // Get our return address, should be just after "bl _init_bats_trampoline"
	oris 3, 3, 0x8000 // now that BATs are enabled, we can start prefixing MEM1 addresses with bit 0 set
	mtsrr0 3
	rfi

_init_bats_trampoline:
	mtsrr0 3 // exception PC (_init_bats)
	mtsrr1 4 // intended machine status
	rfi // jump to _init_bats

_start:
	// disable interrupts, BATs
	mfmsr 4
	rlwinm 6, 4, 0, 17, 15 // clear the EE bit, leaving this version in r6
	rlwinm 4, 6, 0, 28, 25 // disable BATs for instructions and data
	mtmsr 6 // We want to disable BATs as we transition to the exception coming up in _init_bats_trampoline
	isync

	// initialize BATs. We use a "trampoline" of sorts so that we can save the return address using a bl instruction
	lis 3, _init_bats@ha
	rlwinm 3, 3, 0, 1, 31
	addi 3, 3, _init_bats@l
	bl _init_bats_trampoline

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
	addic 3, 24, -4
	addic 8, 26, -4
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
