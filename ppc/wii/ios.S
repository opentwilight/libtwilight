.globl TW_RebootIosSync // bool TW_RebootIosSync(unsigned *iosBufAligned64Bytes)
TW_RebootIosSync:
	// flush iosBuf, since it was just written to
	addi 5, 3, 0x20
	dcbf 0, 3
	dcbf 0, 5
	sync

	lis 6, 0xcd00          // ipc register base
	rlwinm 7, 3, 0, 2, 31  // iosBuf as a physical address
	stw 7, 0(6)            // use ioBuf address as the IPC value
	li 7, 1                // 1 = clear the send flag
	stw 7, 4(6)            // store the new control flags
	eieio                  // sync external io
_tw_reboot_ios_check_ack:
	sync
	lwz 7, 4(6)                 // get the response
	rlwinm. 7, 7, 0, 30, 30     // check if the acknowledge flag (2) is set
	beq _tw_reboot_ios_check_ack  // check again if not

	li 7, 2     // clear the acknowledge flag
	stw 7, 4(6)
	eieio

_tw_reboot_ios_check_ack_again:
	sync
	lwz 7, 4(6)                 // get the response
	rlwinm. 8, 7, 0, 29, 29     // check if the reply flag (4) is set
	bne _tw_reboot_ios_failed
	rlwinm. 8, 7, 0, 30, 30     // check if the acknowledge flag (2) is set
	beq _tw_reboot_ios_check_ack_again  // check again if not

	li 7, 8      // clear the relaunch flag
	stw 7, 4(6)
	eieio

_tw_reboot_ios_check_reply:
	sync
	lwz 7, 4(6)                   // get the response
	rlwinm. 7, 7, 0, 29, 29       // check if the reply flag (4) is set
	beq _tw_reboot_ios_check_reply  // check again if not

	//lwz 8, 8(6) // the loaded value should now match ioBuf

	li 7, 4      // clear the reply flag
	stw 7, 4(6)
	eieio
	li 7, 8      // clear the relaunch flag
	stw 7, 4(6)
	eieio

	// sync iosBuf, since it will (likely) be read once this method returns
	dcbi 0, 3
	dcbi 0, 5
	sync

	li 3, 0
	blr
_tw_reboot_ios_failed:
	li 3, 1
	blr

.globl TW_SubmitIosRequest // void TW_SubmitIosRequest(unsigned *iosBufAligned64Bytes)
TW_SubmitIosRequest:
	// flush iosBuf, since it was just written to
	addi 5, 3, 0x20
	dcbf 0, 3
	dcbf 0, 5
	sync

	lis 6, 0xcd00          // ipc register base
	rlwinm 7, 3, 0, 2, 31  // iosBuf as a physical address
	stw 7, 0(6)            // use ioBuf address as the IPC value
	li 7, 1                // 1 = clear the send flag
	stw 7, 4(6)            // store the new control flags
	eieio                  // sync external io
_tw_submit_ios_check_ack:
	sync
	lwz 7, 4(6)                 // get the response
	rlwinm. 7, 7, 0, 30, 30     // check if the acknowledge flag (2) is set
	beq _tw_submit_ios_check_ack  // check again if not

	li 7, 2     // clear the acknowledge flag
	stw 7, 4(6)
	eieio

	blr

.globl TW_HandleIosInterrupt
TW_HandleIosInterrupt:
	lis 3, 0xcd00
	lwz 4, 4(3)
	rlwinm. 5, 4, 0, 30, 30            // check if the acknowledge flag (2) is set
	beq _tw_ios_check_interrupt_reply  // if its clear, then move on checking the reply flag
	stw 5, 4(3)
	eieio

	lwz 4, 4(3)
_tw_ios_check_interrupt_reply:
	rlwinm. 5, 4, 0, 29, 29            // check if the reply flag (4) is set
	beqlr                              // bail if its clear

	li 5, 4
	stw 5, 4(3)
	eieio
	li 5, 8
	stw 5, 4(3)
	eieio

	lwz 3, 8(3)
	bl TW_InvokeMatchingIosCompletionHandler
	blr
