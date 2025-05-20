#include <twilight_ppc.h>

void TW_SetSerialPollInterval(unsigned line, unsigned count) {
	// kind of a race with TW_SetSerialControllerCount with this register
	unsigned old_pr = PEEK_U32(TW_SERIAL_REG_BASE + 0x30);
	POKE_U32(TW_SERIAL_REG_BASE + 0x30, (old_pr & 0xfc0000ff) | (line << 16) | (count << 8));
}

/*
	Command Word:
		address: TW_SERIAL_REG_BASE + controller * 0xc

	Bits:
		0x00400000 - enable polling
		0x00000300 - ?
		0x00000001 - enable rumble motor
*/

void TW_SetupSerialDevices(unsigned portMask) {
/*
	From YAGCD:

	enable all controllers in 0xcc006430
	if (GameCube controller)
		set Joy-channel N Command Register to 0x00400300
	if (keyboard)
		set Joy-channel N Command Register to 0x00540000
	clear SI i/o buffer
	wait until bit 31 of 0xCC006434 is 0, then set it to 1
*/
	if (portMask == 0)
		return;

	POKE_U32(TW_SERIAL_REG_BASE + 0x3c, 0);
	POKE_U32(TW_SERIAL_REG_BASE + 0x38, 0);	
	POKE_U32(TW_SERIAL_REG_BASE + 0x30, (portMask & 0xf) << 4);
	POKE_U32(TW_SERIAL_REG_BASE + 0x34, 0);

	// TODO: derive 'command' from TW_IdentifySerialController
	unsigned command = 0x00400300;
	unsigned enable = 0;

	for (int i = 0; i < 4; i++) {
		if (portMask & (1 << i)) {
			POKE_U32(TW_SERIAL_REG_BASE + 0xc * i, command);
			enable |= 0x80u << (8 * (3 - i));
		}
	}

	POKE_U32(TW_SERIAL_REG_BASE + 0x38, enable | PEEK_U32(TW_SERIAL_REG_BASE + 0x38));
}

// Rumble on:  *(volatile unsigned long*)0xCC006400 = 0x00400001; *(volatile unsigned long*)0xCC006438 = 0x80000000;
// Rumble off: *(volatile unsigned long*)0xCC006400 = 0x00400000; *(volatile unsigned long*)0xCC006438 = 0x80000000;

/*
	Direct command (32-bits)
	byte 0 | unused
	byte 1 | opcode: 0x30 = FORCE, 0x40 = WRITE, 0x54 = POLL
	byte 2 | data 0
	byte 3 | data 1
*/
unsigned TW_RequestSerialDirect(int port, unsigned command) {
	unsigned enable = 0x80u << (8 * (3 - port));
	POKE_U32(TW_SERIAL_REG_BASE + 0xc * port, command);
	POKE_U32(TW_SERIAL_REG_BASE + 0x38, enable | PEEK_U32(TW_SERIAL_REG_BASE + 0x38));
	return 0;
}

unsigned TW_RequestSerialBuffer(int port, unsigned char *input, int inputLen, unsigned char *output, int outputLen) {
	for (int i = 0; i < outputLen; i += 4) {
		unsigned word = PEEK_U32(TW_SERIAL_REG_BASE + 0x80 + i);
		output[i] = (unsigned char)(word >> 24);
		if (i+1 < outputLen)
			output[i+1] = (unsigned char)(word >> 16);
		if (i+2 < outputLen)
			output[i+2] = (unsigned char)(word >> 8);
		if (i+3 < outputLen)
			output[i+3] = (unsigned char)word;
	}
	return 0;
}

int TW_IdentifySerialController(int port, int *outIsRumble) {
	/*
	The device description and status can be read by sending the SI Command 0x00, and then reading 3 bytes from the respective device.
	Description: 2 bytes (0x0900 for standard GameCube controller)
		wW = wireless, r = has rumble?, g = for GameCube, t = wireless type (IF=0, RF=1), s = semi-wireless, S = standard GameCube controller
		wWr_ gtsS ???? ????
		0123 4567 89ab cdef
	Status: 1 byte
		r = rumble currently on
		???? r???
		0123 4567
	*/
	unsigned command = 0;
	unsigned output = 0;
	unsigned char *output_as_buf = (unsigned char *)&output;
	TW_RequestSerialBuffer(port, (unsigned char*)&command, 1, output_as_buf, 3);

	int desc = (((int)output_as_buf[0] & 0xff) << 8) | ((int)output_as_buf[1] & 0xff);
	if (outIsRumble)
		*outIsRumble = output_as_buf[2] != 0;
	return desc;
}

TwSerialInput TW_GetSerialInputs(int port) {
	TwSerialInput inputs;
	inputs.words[0] = PEEK_U32(TW_SERIAL_REG_BASE + port * 0xc + 4);
	inputs.words[1] = PEEK_U32(TW_SERIAL_REG_BASE + port * 0xc + 8);
	return inputs;
}
