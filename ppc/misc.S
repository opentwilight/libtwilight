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
