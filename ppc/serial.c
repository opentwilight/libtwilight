#include "twilight_ppc.h"

void TW_SetSerialPollInterval(unsigned line, unsigned count) {
	unsigned old_pr = PEEK_U32(TW_SERIAL_REG_BASE + 0x30);
	POKE_U32(TW_SERIAL_REG_BASE + 0x30, (old_pr & 0xfc0000ff) | (line << 16) | (count << 8));
}
