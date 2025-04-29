#include "twilight.h"

static int _initialized = 0;
static char *_heap_ptr = (char*)0x80010000;

static void init_twilight() {
	
}

void TW_TwilightInit(void) {
	if (!_initialized) {
		init_twilight();
		_initialized = 1;
	}
}

// TODO: Use a proper heap allocator, not just a simple bump allocator

void *TW_Allocate(int count, int elem_size) {
	char *old_heap_ptr = _heap_ptr;
	_heap_ptr = &old_heap_ptr[(count * elem_size + 7) & ~7];
	return old_heap_ptr;
}

void TW_Free(void *ptr) {
	// no-op
}
