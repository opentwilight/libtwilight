.globl TW_LoadFprs // void TW_LoadFprs(double *buf)
TW_LoadFprs:
	lfd 0, 0(3)
	lfd 1, 8(3)
	lfd 2, 0x10(3)
	lfd 3, 0x18(3)
	lfd 4, 0x20(3)
	lfd 5, 0x28(3)
	lfd 6, 0x30(3)
	lfd 7, 0x38(3)
	lfd 8, 0x40(3)
	lfd 9, 0x48(3)
	lfd 10, 0x50(3)
	lfd 11, 0x58(3)
	lfd 12, 0x60(3)
	lfd 13, 0x68(3)
	lfd 14, 0x70(3)
	lfd 15, 0x78(3)
	lfd 16, 0x80(3)
	lfd 17, 0x88(3)
	lfd 18, 0x90(3)
	lfd 19, 0x98(3)
	lfd 20, 0xa0(3)
	lfd 21, 0xa8(3)
	lfd 22, 0xb0(3)
	lfd 23, 0xb8(3)
	lfd 24, 0xc0(3)
	lfd 25, 0xc8(3)
	lfd 26, 0xd0(3)
	lfd 27, 0xd8(3)
	lfd 28, 0xe0(3)
	lfd 29, 0xe8(3)
	lfd 30, 0xf0(3)
	lfd 31, 0xf8(3)
	blr

.globl TW_SaveFprs // void TW_SaveFprs(double *buf)
TW_SaveFprs:
	stfd 0, 0(3)
	stfd 1, 8(3)
	stfd 2, 0x10(3)
	stfd 3, 0x18(3)
	stfd 4, 0x20(3)
	stfd 5, 0x28(3)
	stfd 6, 0x30(3)
	stfd 7, 0x38(3)
	stfd 8, 0x40(3)
	stfd 9, 0x48(3)
	stfd 10, 0x50(3)
	stfd 11, 0x58(3)
	stfd 12, 0x60(3)
	stfd 13, 0x68(3)
	stfd 14, 0x70(3)
	stfd 15, 0x78(3)
	stfd 16, 0x80(3)
	stfd 17, 0x88(3)
	stfd 18, 0x90(3)
	stfd 19, 0x98(3)
	stfd 20, 0xa0(3)
	stfd 21, 0xa8(3)
	stfd 22, 0xb0(3)
	stfd 23, 0xb8(3)
	stfd 24, 0xc0(3)
	stfd 25, 0xc8(3)
	stfd 26, 0xd0(3)
	stfd 27, 0xd8(3)
	stfd 28, 0xe0(3)
	stfd 29, 0xe8(3)
	stfd 30, 0xf0(3)
	stfd 31, 0xf8(3)
	blr

.globl TW_SwitchContext // void TW_SwitchContext(TW_PpcCpuContext *fromCtx, TW_PpcCpuContext *toCtx)
TW_SwitchContext:
	stmw 0, 0(3)

	mflr 26 // save program counter as LR
	mflr 27
	mfxer 28
	mfctr 29
	mfcr 30

	addi 3, 3, 0x80
	bl TW_SaveFprs

	addic 1, 1, -8
	mffs 0
	stfd 0, 0(1)
	lwz 31, 4(1) // Save FPSCR
	addi 1, 1, 8

	stmw 26, 0x100(3)

	lfd 0, 0x194(4)
	mtfsf 0xff, 0 // Restore FPSCR

	addi 3, 4, 0x80
	bl TW_LoadFprs

	lmw 26, 0x180(4)
	mtlr 27
	mtxer 28
	mtctr 29
	mtcr 30

	li 5, 0x2032
	mtsrr0 26
	mtsrr1 5

	lmw 0, 0(4)
	rfi

.globl TW_EnterContext  // void TW_EnterContext(TW_PpcCpuContext *ctx)
TW_EnterContext:
	lfd 0, 0x194(3)
	mtfsf 0xff, 0 // Restore FPSCR

	mr 5, 3
	addi 3, 3, 0x80
	bl TW_LoadFprs
	mr 3, 5

	lmw 26, 0x180(3)
	mtlr 27
	mtxer 28
	mtctr 29
	mtcr 30

	li 5, 0x2032
	mtsrr0 26
	mtsrr1 5

	lmw 0, 0(3)
	rfi

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
	stw 1, -0x190(4)
	addic 1, 4, -0x194
	stw 0, 0(1)
	stw 2, 8(1)

	mfspr 4, 272
	stw 4, 16(1) // r4

	lis 4, __tw_interrupt_call_scratch@ha
	addi 4, 4, __tw_interrupt_call_scratch@l

	lwz 5, 4(4)   // r5 got clobbered earlier, so restore it here before saving
	stmw 5, 20(1)

	lwz 27, 16(4) // LR

	lwz 4, 0(4)
	stw 4, 12(1) // r3

	mr 5, 3
	addi 3, 1, 0x80
	bl TW_SaveFprs
	mr 3, 5

	mfxer 28
	mfctr 29
	mfcr 30

	addic 1, 1, -8
	mffs 0
	stfd 0, 0(1)
	lwz 31, 4(1) // Load FPSCR
	addi 1, 1, 8

	stmw 27, 0x180(1)

	// For some reason, the stack gets accessed out of bounds by _tw_dispatch_interrupt() when compiled with Clang.
	// Usually this sort of thing is not too concerning, but since this OoB access is a write that overwrites the saved stack pointer,
	// it causes big issues. So as a workaround, we create a buffer zone between the saved regs and the stack.
	addic 1, 1, -32
	bl _tw_dispatch_interrupt
	addi 1, 1, 32

	lmw 27, 0x180(1)
	mtxer 28
	mtctr 29
	mtcr 30

	addic 1, 1, -8
	stw 31, 4(1)
	lfd 0, 0(1)
	mtfsf 0xff, 0 // Restore FPSCR
	addi 1, 1, 8

	addi 3, 1, 0x80
	bl TW_LoadFprs

	lwz 0, 0(1)
	lwz 2, 8(1)
	lmw 5, 20(1)
	lwz 1, 4(1)

	lis 4, __tw_interrupt_call_scratch@ha
	addi 4, 4, __tw_interrupt_call_scratch@l

	lwz 3, 8(4)
	mtsrr0 3
	lwz 3, 12(4)
	mtsrr1 3
	lwz 3, 16(4)
	mtlr 3
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
	mfsrr0 3
	stw 3, 8(4)
	mfsrr1 3
	stw 3, 12(4)
	mflr 3
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

