#include "twilight.h"

extern unsigned __stack_bottom;
extern unsigned __stack_size;

static void *_heap_ptr;

void TW_TwilightInit(void) {
	unsigned stack_top = __stack_bottom + __stack_size;
	_heap_ptr = (void*)stack_top;
}

// TODO: Use a proper heap allocator, not just a simple bump allocator

void *TW_Allocate(int count, int elem_size) {
	int size = count * elem_size;
	if (size == 0)
		return (void*)0;

	unsigned alloc_size = (unsigned)((size + 7) & ~7);
	unsigned old_addr = (unsigned)_heap_ptr;

#if TW_WII
	if (old_addr <= MEM1_END && old_addr + alloc_size > MEM1_END)
		old_addr = MEM2_START;
	if (old_addr + alloc_size > IOS_MEM_START)
		return (void*)0;
#else
	if (old_addr + alloc_size > MEM1_END)
		return (void*)0;
#endif

	_heap_ptr = (void*)(old_addr + alloc_size);
	return (void*)old_addr;
}

void TW_Free(void *ptr) {
	// no-op
}
