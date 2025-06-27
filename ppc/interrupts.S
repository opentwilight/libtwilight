.globl TW_GetStoredRegistersAddress
TW_GetStoredRegistersAddress:
	bl TW_GetInterruptStackStart
	addic 3, 3, -208
	blr

__tw_interrupt_call_scratch:
	.long 0 // r3
	.long 0 // r5
	.long 0 // LR
	.long 0 // SRR0
	.long 0 // SRR1

.globl __tw_external_interrupt_handler_table
__tw_external_interrupt_handler_table:
	.fill 48, 4, 0

.globl __tw_cpu_interrupt_handler_table
__tw_cpu_interrupt_handler_table:
	.fill 32, 4, 0

// void _tw_interrupt_handler_impl(unsigned exceptionType, void *stackPointer)
_tw_interrupt_handler_impl:
	stw 1, -204(4)
	addic 1, 4, -208
	stw 0, 0(1)
	stw 2, 8(1)

	mfspr 4, 272
	stw 4, 16(1) // r4

	lis 4, __tw_interrupt_call_scratch@ha
	addi 4, 4, __tw_interrupt_call_scratch@l

	lwz 5, 4(4)   // r5 got clobbered earlier, so restore it here before saving
	stmw 5, 20(1)

	lwz 17, 8(4)  // LR
	lwz 18, 12(4) // SRR0
	lwz 19, 16(4) // SRR1

	lwz 4, 0(4)
	stw 4, 12(1) // r3

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
	stmw 17, 132(1)

	// For some reason, the stack gets accessed out of bounds by _tw_dispatch_interrupt() when compiled with Clang.
	// Usually this sort of thing is not too concerning, but since this OoB access is a write that overwrites the saved stack pointer,
	// it causes big issues. So as a workaround, we create a buffer zone between the saved regs and the stack.
	addic 1, 1, -32
	bl _tw_dispatch_interrupt
	addi 1, 1, 32

	lmw 17, 132(1)
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

	lwz 0, 0(1)
	lwz 2, 8(1)
	lmw 5, 20(1)
	lwz 1, 4(1)

	lis 4, __tw_interrupt_call_scratch@ha
	addi 4, 4, __tw_interrupt_call_scratch@l

	lwz 3, 8(4)
	mtlr 3
	lwz 3, 12(4)
	mtsrr0 3
	lwz 3, 16(4)
	mtsrr1 3
	lwz 3, 0(4)

	mfspr 4, 272 // Restore r4 register from SPRG0, a backup register that we placed the original contents of r4 in
	rfi

.globl _tw_interrupt_handler_template
_tw_interrupt_handler_template:
	// Save r4 register to SPRG0 (a backup register) ...
	mtspr 272, 4

	// ...allowing us to use r4 as a pointer to save everything else with
	lis 4, __tw_interrupt_call_scratch@ha
	rlwinm 4, 4, 0, 1, 31 // convert address to physical, since BATs are disabled when a hardware interrupt is issued
	addi 4, 4, __tw_interrupt_call_scratch@l

	// Save r3, r5, LR, SRR0 and SRR1, since we have to clobber these registers before we can move on.
	// We use a stwcx. instruction to attempt to store r3 to [r4] first,
	//  since this instruction is the only way to clear the memory reservation flag.
	// If that flag was set, then the store operation succeeds, otherwise, it never goes through.
	// We want to clear that flag regardless as that is how we communicate interruption to an atomic operation.
	// So, regardless of whether stwcx. "worked" or not, we will store the value again anyway.
	stwcx. 3, 0, 4
	stw 3, 0(4)
	stw 5, 4(4)
	mflr 3
	stw 3, 8(4)
	mfsrr0 3
	stw 3, 12(4)
	mfsrr1 3
	stw 3, 16(4)

	// This does two things:
	//  - it gets us the stack pointer in r3 (while clobbering r4)
	//  - it stores the current instruction address in LR
	lis 3, TW_GetInterruptStackStart@ha
	rlwinm 3, 3, 0, 1, 31
	addi 3, 3, TW_GetInterruptStackStart@l
	mtlr 3
	blrl
	mr 4, 3

	// Get the type of exception as an index
	mflr 3 // a nearby instruction address, which is now in LR, is dependent on which kind of exception occurred. Save it to r3.
	rlwinm 3, 3, 24, 24, 31 // shift right by 8 (rotate left by 24) then keep only the 8 least significant bits

	// Call _tw_interrupt_handler_impl as a software interrupt with the exception type in r3 and the intended stack pointer in r4
	lis 5, _tw_interrupt_handler_impl@ha
	addi 5, 5, _tw_interrupt_handler_impl@l
	mtsrr0 5
	li 5, 0x2032 // floating point, BATs, recover from exceptions
	mtsrr1 5
	rfi

.globl _tw_interrupt_handler_template_end
_tw_interrupt_handler_template_end:
	.long 0

