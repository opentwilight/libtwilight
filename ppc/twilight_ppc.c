#include "twilight_ppc.h"

extern unsigned __toc_region;
extern unsigned __toc_size;
extern unsigned __stack_size;

static int _initialized = 0;
static int _tw_is_mem1_full = 0;

static TwHeapAllocator _g_tw_heap_a;
#if TW_WII
static TwHeapAllocator _g_tw_heap_b;
#endif

static void init_twilight() {
	int fb_size;
	unsigned framebuffer = TW_GetFramebufferAddress(&fb_size);
	_g_tw_heap_a = TW_MakeHeap((void*)(framebuffer + fb_size), (void*)TW_MEM1_END);
#if TW_WII
	_g_tw_heap_b = TW_MakeHeap((void*)TW_MEM2_START, (void*)TW_IOS_MEM_START);
#endif
}

void TW_InitTwilight(void) {
	if (!_initialized) {
		init_twilight();
		_initialized = 1;
	}
}

unsigned TW_GetFramebufferAddress(int *outSize) {
	unsigned stack_top = (unsigned)&__toc_region + __toc_size + __stack_size;
	unsigned framebuffer = (stack_top + 0x1ff) & ~0x1ff;
	int fb_size = 2 * 640 * 576;
	if (outSize)
		*outSize = fb_size;
	return framebuffer;
}

// TODO: Add mutexes

static void *_tw_allocate_global(int count, int elemSize) {
	void *ptr = (void*)0;
#if TW_WII
	if (!_tw_is_mem1_full) {
		ptr = TW_AllocateHeapObject(&_g_tw_heap_a, count, elemSize);
		if (!ptr) {
			int size = TW_CalcHeapObjectInnerSize(count, elemSize);
			if (size <= 256)
				_tw_is_mem1_full = 1;
		}
	}
	if (!ptr) {
		ptr = TW_AllocateHeapObject(&_g_tw_heap_b, count, elemSize);
	}
#else	
	ptr = TW_AllocateHeapObject(&_g_tw_heap_a, count, elemSize);
#endif
	return ptr;
}

void *TW_AllocateGlobal(int count, int elemSize) {
	// Lock mutex here
	void *ptr = _tw_allocate_global(count, elemSize);
	// Unlock mutex here
	return ptr;
}

static void _tw_free_global(void *ptr) {
#if TW_WII
	TwHeapAllocator *alloc = (unsigned)ptr < TW_MEM2_START ? &_g_tw_heap_a : &_g_tw_heap_b;
	TW_FreeHeapObject(alloc, ptr);
#else
	TW_FreeHeapObject(&_g_tw_heap_a, ptr);
#endif
}

void TW_FreeGlobal(void *ptr) {
	// Lock mutex here
	_tw_free_global(ptr);
	// Unlock mutex here
}

static void *_tw_reallocate_global(void *ptr, int count, int elemSize) {
	int cur_size = TW_RetrieveHeapObjectInnerSize((void*)0, ptr);
	int new_size = TW_CalcHeapObjectInnerSize(count, elemSize);
	if (new_size <= cur_size) {
		TW_UpdateHeapObjectSize((void*)0, ptr, new_size);
		return ptr;
	}

	int space_until_next = TW_GetSpaceUntilNextOccupiedHeapObject((void*)0, ptr);
	if (space_until_next < 0 || new_size < space_until_next) {
		TW_UpdateHeapObjectSize((void*)0, ptr, new_size);
		return ptr;
	}

	void *new_ptr = _tw_allocate_global(count, elemSize);
	if (!new_ptr)
		return (void*)0;

	TW_CopyBytes(new_ptr, ptr, cur_size);
	_tw_free_global(ptr);
	return new_ptr;
}

void *TW_ReallocateGlobal(void *ptr, int count, int elemSize) {
	// Lock mutex here
	void *new_ptr = _tw_reallocate_global(ptr, count, elemSize);
	// Unlock mutex here
	return new_ptr;
}
