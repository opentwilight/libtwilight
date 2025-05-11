.globl TW_EnableInterrupts
TW_EnableInterrupts: // void TW_EnableInterrupts()
	mfmsr 3
	ori 3, 3, 0x8000 // set the EE bit
	mtmsr 3
	isync
	blr

.globl TW_DisableInterrupts // void TW_DisableInterrupts()
TW_DisableInterrupts:
	mfmsr 3
	rlwinm 3, 3, 0, 17, 15 // clear the EE bit
	mtmsr 3
	isync
	blr

.globl TW_FlushMemory // void *TW_FlushMemory(void *ptr, int size)
TW_FlushMemory:
	addi 3, 3, 0x1f
	addi 4, 4, 0x1f
	rlwinm 3, 3, 0, 0, 26
	rlwinm 4, 4, 0, 0, 26
	add 5, 3, 4
_tw_flush_memory_loop:
	dcbf 0, 3
	dcbt 0, 3
	dcbst 0, 3
	sync
	icbi 0, 3
	addi 3, 3, 0x20
	cmpw 0, 3, 5
	bne+ _tw_flush_memory_loop
	blr

.globl __tw_external_interrupt_handler_table
__tw_external_interrupt_handler_table:
	.fill 16, 4, 0

.globl __tw_cpu_interrupt_handler_table
__tw_cpu_interrupt_handler_table:
	.fill 32, 4, 0

_tw_interrupt_handler_impl:
	stwu 1, -192(1)
	stw 0, 4(1)
	stw 2, 8(1)
	stmw 5, 12(1)

	mfxer 20
	mfctr 21
	mfcr 22
	mfdar 23
	mfspr 24, 912
	mfspr 25, 913
	mfspr 26, 914
	mfspr 27, 915
	mfspr 28, 916
	mfspr 29, 917
	mfspr 30, 918
	mfspr 31, 919
	stmw 20, 120(1)

	bl _tw_dispatch_interrupt

	lmw 20, 120(1)
	mtxer 20
	mtctr 21
	mtcr 22
	mtdar 23
	mtspr 912, 24
	mtspr 913, 25
	mtspr 914, 26
	mtspr 915, 27
	mtspr 916, 28
	mtspr 917, 29
	mtspr 918, 30
	mtspr 919, 31

	lwz 0, 4(1)
	lwz 2, 8(1)
	lmw 5, 12(1)
	lwz 1, 0(1)

	lis 4, _tw_interrupt_call_scratch@ha
	addi 4, 4, _tw_interrupt_call_scratch@l

	lwz 3, 4(4)
	mtlr 3
	lwz 3, 8(4)
	mtsrr0 3
	lwz 3, 12(4)
	mtsrr1 3
	lwz 3, 0(4)

	mfspr 4, 272 // Restore r4 register from SPRG0, a backup register that we placed the original contents of r4 in
	rfi

_tw_interrupt_call_scratch:
	.long 0 // r3
	.long 0 // LR
	.long 0 // SRR0
	.long 0 // SRR1

.globl _tw_interrupt_handler_template
_tw_interrupt_handler_template:
	// Save r4 register to SPRG0 (a backup register) ...
	mtspr 272, 4

	// ...allowing us to use r4 as a pointer to save everything else with
	lis 4, _tw_interrupt_call_scratch@ha
	rlwinm 4, 4, 0, 1, 31 // convert address to physical, since BATs are disabled when a hardware interrupt is issued
	addi 4, 4, _tw_interrupt_call_scratch@l

	// Save r3, LR, SRR0 and SRR1, since we have to clobber these registers before we can move on
	stw 3, 0(4)
	mflr 3
	stw 3, 4(4)
	mfsrr0 3
	stw 3, 8(4)
	mfsrr1 3
	stw 3, 12(4)

	// Get the type of exception as an index
	bl 4 // Branch to the next instruction, while saving the address to the LR
	mflr 3 // Get the LR, which is dependent on which kind of exception occurred
	rlwinm 3, 3, 24, 24, 31 // shift right by 8 (rotate left by 24) then keep only the 8 least significant bits

	lis 4, _tw_interrupt_handler_impl@ha
	addi 4, 4, _tw_interrupt_handler_impl@l
	mtsrr0 4
	li 4, 0x2032 // floating point, BATs, recover from exceptions
	mtsrr1 4
	rfi

.globl _tw_interrupt_handler_template_end
_tw_interrupt_handler_template_end:
	.long 0
