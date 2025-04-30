	.globl _start
_start:
	mfmsr 3
	rlwinm 3, 3, 0, 17, 15
	mtmsr 3
	isync

	lis 1, _stack_top@ha
	ori 1, 1, _stack_top@l
	li 0, 0
	mr 3, 0
	stwu 3, -64(1)

	bl main
	b .
