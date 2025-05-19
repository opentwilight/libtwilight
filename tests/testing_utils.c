#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/twilight_common.h"

#define TEST_HEAP_SIZE 0x40000
TwHeapAllocator globalAlloc = {};

void initTestingGlobalAllocator() {
	char *heap = malloc(TEST_HEAP_SIZE);
	globalAlloc = TW_MakeHeapAllocator(heap, &heap[TEST_HEAP_SIZE]);
}

TwHeapAllocator *TW_GetGlobalAllocator() { return &globalAlloc; }
void *TW_CopyBytes(void *dst, const void *src, int nBytes) { return memmove(dst, src, nBytes); }
unsigned TW_DisableInterrupts() { return 0; }
unsigned TW_EnableInterrupts() { return 0; }
