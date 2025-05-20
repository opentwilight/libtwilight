#include <twilight_common.h>

int TW_MultiThreadingEnabled(void) {
	return 0;
}

void TW_LockMutex(void **mutex) {
	unsigned value = (unsigned)*mutex;
	if (value >= 1 && value < 0x10000) {
		if (value == 1)
			TW_DisableInterrupts();
		*mutex = (void*)(value + 1);
		return;
	}

	// TODO: actual mutex lock goes here
}

void TW_UnlockMutex(void **mutex) {
	unsigned value = (unsigned)*mutex;
	if (value >= 2 && value <= 0x10000) {
		if (value == 2)
			TW_EnableInterrupts();
		*mutex = (void*)(value - 1);
		return;
	}

	// TODO: actual mutex unlock goes here
}
