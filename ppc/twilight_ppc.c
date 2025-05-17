#include "twilight_ppc.h"

extern unsigned __toc_region;
extern unsigned __toc_size;
extern unsigned __stack_size;

static int _initialized = 0;

static TwHeapAllocator _g_tw_heap;
#if TW_WII
static TwHeapAllocator _g_tw_heap_ex;
#endif

static void init_twilight() {
	int fb_size;
	unsigned framebuffer = TW_GetFramebufferAddress(&fb_size);
	_g_tw_heap = TW_MakeHeap((void*)(framebuffer + fb_size), (void*)TW_MEM1_END);
	_g_tw_heap.mutex = (void*)1; // use interrupt toggle as the lock, ie. global
#if TW_WII
	_g_tw_heap_ex = TW_MakeHeap((void*)TW_MEM2_START, (void*)TW_IOS_MEM_START);
	_g_tw_heap.next = &_g_tw_heap_ex;
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

TwHeapAllocator *TW_GetGlobalAllocator(void) {
	return &_g_tw_heap;
}
