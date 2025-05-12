#include "twilight_ppc.h"

void TW_SetSerialPollInterval(unsigned line, unsigned count) {
	unsigned old_pr = PEEK_U32(TW_SERIAL_REG_BASE + 0x30);
	POKE_U32(TW_SERIAL_REG_BASE + 0x30, (old_pr & 0xfc0000ff) | (line << 16) | (count << 8));
}

void TW_InitSerial() {
	POKE_U32(TW_SERIAL_REG_BASE + 0x3c, 0);
	POKE_U32(TW_SERIAL_REG_BASE + 0x30, 0x10);
	POKE_U32(TW_SERIAL_REG_BASE + 0x38, 0);
	POKE_U32(TW_SERIAL_REG_BASE + 0x34, 0);
	// TODO
}

void TW_SetSerialControllerCount(int nControllers) {
	// TODO
}
