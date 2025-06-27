// __toc_size = 0x8000
// __interrupt_stack_size = 0x10000
// __stack_size = 0x30000

.globl TW_GetTocPointer
.globl TW_GetInterruptStackStart
.globl TW_GetMainStackStart
TW_GetMainStackStart:
	li 3, 0x480
	b _tw_get_core_pointer_impl
TW_GetInterruptStackStart:
	li 3, 0x180
	b _tw_get_core_pointer_impl
TW_GetTocPointer:
	li 3, 0
_tw_get_core_pointer_impl:
	slwi 3, 3, 8
	lis 4, __toc_region@ha
	addi 4, 4, __toc_region@l
	add 3, 4, 3
	blr

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

.globl TW_CompareAndSwapAtomic // bool TW_CompareAndSwapAtomic(unsigned *address, unsigned expectedMask, unsigned expectedValue, unsigned newValue)
TW_CompareAndSwapAtomic:
	sync
	isync
	// reserve the 32 byte cache block, so that we know if we were interrupted before we could complete the swap
	lwarx 7, 0, 3
	// select newValue if r7 == expected, else keep the old value
	and 9, 7, 4             // bitwise AND the loaded value with the expected mask
	and 8, 5, 4             // bitwise AND the expected value with the expected mask
	sub 8, 9, 8             // get the difference between the two
	neg 9, 8                // negate and OR ensures the top bit of is set if and only if there is a difference
	or 8, 8, 9
	rlwinm 8, 8, 1, 31, 31  // shift right by 31, so we have it in the least significant bit
	neg 9, 8                // set r9 to all ones if there is a difference, all zeros if not
	addi 10, 9, 1           // keep whether it was equal or not in r10 by adding 1 to the not-equal mask
	addic 8, 8, -1          // set r8 to all ones if there is NO difference, all zeros if so
	and 8, 8, 6             // valueToSet = (newValue & matchedExpected) | (oldValue & didNotMatchExpected)
	and 9, 9, 7
	or 7, 9, 8
	stwcx. 7, 0, 3          // attempt to store if the earlier reservation still exists
	bne TW_CompareAndSwapAtomic
	mr 3, 10                 // we kept whether the old value matched the expected in r10, so copy it to the return register
	blr

.globl TW_GetAndSetAtomic // unsigned TW_GetAndSetAtomic(unsigned *address, unsigned newValue)
TW_GetAndSetAtomic:
	sync
	isync
	lwarx 6, 0, 3
	stwcx. 4, 0, 3
	bne TW_GetAndSetAtomic
	mr 3, 6
	blr

.globl TW_AddAtomic // unsigned TW_AddAtomic(unsigned *address, int delta)
TW_AddAtomic:
	sync
	isync
	lwarx 6, 0, 3
	add 7, 6, 4
	stwcx. 7, 0, 3
	bne TW_AddAtomic
	mr 3, 6
	blr

.globl TW_OrAtomic // unsigned TW_OrAtomic(unsigned *address, unsigned value)
TW_OrAtomic:
	sync
	isync
	lwarx 6, 0, 3
	or 7, 6, 4
	stwcx. 7, 0, 3
	bne TW_OrAtomic
	mr 3, 6
	blr

.globl TW_XorAtomic // unsigned TW_XorAtomic(unsigned *address, unsigned value)
TW_XorAtomic:
	sync
	isync
	lwarx 6, 0, 3
	xor 7, 6, 4
	stwcx. 7, 0, 3
	bne TW_XorAtomic
	mr 3, 6
	blr

.globl TW_AndAtomic // unsigned TW_AndAtomic(unsigned *address, unsigned value)
TW_AndAtomic:
	sync
	isync
	lwarx 6, 0, 3
	and 7, 6, 4
	stwcx. 7, 0, 3
	bne TW_AndAtomic
	mr 3, 6
	blr

.globl TW_ZeroAndFlushBlock // void TW_ZeroAndFlushBlock(unsigned *cacheBlockAlignedAddress)
TW_ZeroAndFlushBlock:
	mr 4, 30
	mr 5, 31
	li 24, 0
	li 25, 0
	li 26, 0
	li 27, 0
	li 28, 0
	li 29, 0
	li 30, 0
	li 31, 0
	dcbt 0, 3
	stmw 24, 0(3)
	dcbf 0, 3
	dcbst 0, 3
	sync
	mr 30, 4
	mr 31, 5
	blr

.globl TW_FlushMemory // void *TW_FlushMemory(void *ptr, int size)
TW_FlushMemory:
	add 5, 3, 4
	addi 5, 5, 0x1f
	rlwinm 3, 3, 0, 0, 26
	rlwinm 5, 5, 0, 0, 26
_tw_flush_memory_loop:
	dcbf 0, 3
	dcbt 0, 3
	dcbst 0, 3
	sync
	icbi 0, 3
	isync
	addi 3, 3, 0x20
	cmpw 0, 3, 5
	bne+ _tw_flush_memory_loop
	blr

.globl TW_SyncAfterWrite // void *TW_SyncAfterWrite(void *ptr, int size)
TW_SyncAfterWrite:
	add 5, 3, 4
	addi 5, 5, 0x1f
	rlwinm 3, 3, 0, 0, 26
	rlwinm 5, 5, 0, 0, 26
_tw_sync_after_write_loop:
	dcbf 0, 3
	addi 3, 3, 0x20
	cmpw 0, 3, 5
	bne+ _tw_sync_after_write_loop
	sync
	isync
	blr

.globl TW_SyncBeforeRead // void *TW_SyncBeforeRead(void *ptr, int size)
TW_SyncBeforeRead:
	add 5, 3, 4
	addi 5, 5, 0x1f
	rlwinm 3, 3, 0, 0, 26
	rlwinm 5, 5, 0, 0, 26
_tw_sync_before_read_loop:
	dcbi 0, 3
	addi 3, 3, 0x20
	cmpw 0, 3, 5
	bne+ _tw_sync_after_write_loop
	sync
	isync
	blr

.globl TW_FillWordsAndFlush
TW_FillWordsAndFlush:
	rlwinm. 6, 3, 0, 27, 31
	rlwinm 6, 5, 29, 3, 31
	beq+ _tw_fill_words_sync_loop_bulk_start
	stw 4, 0(3)
	addi 3, 3, 4
	addic 5, 5, -1
	b TW_FillWordsAndFlush
_tw_fill_words_sync_loop_bulk_start:
	mr 24, 4
	mr 25, 4
	mr 26, 4
	mr 27, 4
	mr 28, 4
	mr 29, 4
	mr 30, 4
	mr 31, 4
	mr 7, 6
	cmpwi 0, 5, 32
	blt- _tw_fill_words_sync_loop_post
_tw_fill_words_sync_loop_bulk:
	dcbt 0, 3
	stmw 24, 0(3)
	dcbf 0, 3
	dcbst 0, 3
	sync
	addi 3, 3, 0x20
	addic. 6, 6, -1
	bne+ _tw_fill_words_sync_loop_bulk
	rlwinm 6, 7, 3, 0, 28
	sub. 5, 5, 6
_tw_fill_words_sync_loop_post:
	blelr
	stw 4, 0(3)
	addi 3, 3, 4
	addic. 5, 5, -1
	b _tw_fill_words_sync_loop_post

.globl TW_CopyBytes
TW_CopyBytes:
	cmpwi 0, 5, 0
	blelr
	sub. 6, 3, 4
	beqlr
	blt _tw_copy_bytes_forward
	cmpw 0, 6, 5
	bge _tw_copy_bytes_forward
	mr 7, 3
	add 3, 3, 5
	add 4, 4, 5
_tw_copy_bytes_backward_loop:
	lbzu 6, -1(4)
	stbu 6, -1(3)
	cmpw 0, 3, 7
	bgt _tw_copy_bytes_backward_loop
	add 3, 7, 5
	blr
_tw_copy_bytes_forward:
	addic 3, 3, -1
	addic 4, 4, -1
	add 7, 3, 5
_tw_copy_bytes_forward_loop:
	lbzu 6, 1(4)
	stbu 6, 1(3)
	cmpw 0, 3, 7
	blt _tw_copy_bytes_forward_loop
	blr

.globl TW_FillBytes
TW_FillBytes:
	addic 3, 3, -1
	add 6, 3, 5
_tw_fill_bytes_loop:
	stbu 4, 1(3)
	cmpw 0, 3, 6
	blt _tw_fill_bytes_loop
	blr

.globl TW_CountLeadingZeros
TW_CountLeadingZeros:
	cntlzw 3, 3
	blr

.globl TW_CountBits
TW_CountBits:
	li 5, 0
_tw_count_bits_loop:
	cntlzw 4, 3
	addi 4, 4, 1
	slw. 3, 3, 4
	addi 5, 5, 1
	beq- _tw_count_bits_loop
	mr 3, 5
	blr

.globl TW_HasOneBitSet
TW_HasOneBitSet:
	cntlzw 4, 3
	addi 4, 4, 1
	slw 3, 3, 4
	cntlzw 4, 3
	rlwinm 3, 4, 28, 0, 31
	blr

.globl TW_U32ToDouble
TW_U32ToDouble:
	stwu 1, -16(1)
	stw 3, 8(1)
	lis 4, 0x4330
	stw 4, 4(1)
	lfd 0, 4(1)
	lis 4, 0x5980
	stw 4, 12(1)
	lfs 1, 12(1)
	fsub 1, 0, 1
	addi 1, 1, 16
	blr

.globl TW_ModuloDouble // returns the integer division result in r3, and the normalized modulo in f1
TW_ModuloDouble:
	stwu 1, -16(1)
	mflr 0
	stw 0, 4(1)
	mr 5, 3
	fdiv 3, 1, 2
	fctiwz 4, 3
	stfs 4, 8(1)
	lwz 3, 8(1)
	bl TW_U32ToDouble
	fsub 1, 3, 1 // normalized modulo (0.0-1.0)
	lwz 0, 4(1)
	mtlr 0
	addi 1, 1, 16
	blr

.globl TW_DivideU64 // unsigned TW_DivideU64(unsigned long long *value, unsigned div)
TW_DivideU64:
	stwu 1, -16(1)
	mflr 0
	stw 0, 4(1)
	mr 6, 3
	mr 5, 4
	lwz 3, 0(6)
	lwz 4, 4(6)
	divwu 7, 3, 5
	mullw 8, 7, 5
	sub 8, 3, 8
	divwu 9, 4, 5
	mullw 10, 9, 5
	sub 10, 4, 10
	cmpwi 0, 8, 0
	beq _tw_divu64_after_upper_rem // if there's no remainder in the upper 32 bit division, then skip
	mr 3, 5
	bl TW_U32ToDouble // convert the divisor to a double
	fmr 2, 1
	mr 3, 8
	bl TW_U32ToDouble // convert the upper 32-bit remainder to a double
	fmr 3, 1
	lis 3, 0x4f80
	stw 3, 8(1)
	lfs 1, 8(1)
	fmul 1, 1, 3
	bl TW_ModuloDouble // divide f1 by f2, with the division result going to r3 and the normalized modulo going to f1
	add 9, 9, 3 // add the division result as an integer to the lower 32-bit divsion result
	fmul 1, 1, 2 // un-normalize our modulo
	fctiwz 3, 1
	stfs 3, 12(1)
	lwz 4, 12(1) // get it in r4 as an integer
	add 10, 10, 4 // add to our overall remainder
_tw_divu64_after_upper_rem:
	stw 7, 0(6)
	stw 9, 4(6)
	mr 3, 10
	lwz 0, 4(1)
	mtlr 0
	addi 1, 1, 16
	blr

