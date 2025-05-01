#include "twilight.h"

typedef struct {
	int size; // size is negative if this block has been freed
	void *prev;
} TW_DataHeapAlloc;

extern unsigned __stack_bottom;
extern unsigned __stack_size;

static void *_heap_ptr;
static void *_prev_heap_ptr;

void TW_TwilightInit(void) {
	unsigned stack_top = __stack_bottom + __stack_size;
	_heap_ptr = (void*)(stack_top + sizeof(TW_DataHeapAlloc));
	_prev_heap_ptr = (void*)0;
}

// TODO: Add mutexes
// TODO: Look at previous heap entries for until there's enough space for the new entry,
//        but look past the first free entry in case there's contiguous free blocks that take up enough space together

void *TW_Allocate(int count, int elem_size) {
	int size = count * elem_size;
	if (size <= 0)
		return (void*)0;

	// Lock mutex here

	unsigned alloc_size = (unsigned)((size + 7) & ~7);
	unsigned old_addr = (unsigned)_heap_ptr;

	int failed = 0;

#if TW_WII
	if (old_addr <= MEM1_END && old_addr + alloc_size > MEM1_END)
		old_addr = MEM2_START + sizeof(TW_DataHeapAlloc);
	if (old_addr + alloc_size > IOS_MEM_START)
		failed = 1;
#else
	if (old_addr + alloc_size > MEM1_END)
		failed = 1;
#endif

	if (!failed) {
		TW_DataHeapAlloc *obj = &((TW_DataHeapAlloc*)old_addr)[-1];
		obj->size = size;
		obj->prev = _prev_heap_ptr;
		_prev_heap_ptr = _heap_ptr;
		_heap_ptr = (void*)(old_addr + alloc_size);

		// Unlock here
		return (void*)old_addr;
	}
	else {
		// Unlock here
		return (void*)0;
	}
}

void TW_Free(void *ptr) {
	TW_DataHeapAlloc *obj = &((TW_DataHeapAlloc*)ptr)[-1];
	obj->size = -obj->size;
}
