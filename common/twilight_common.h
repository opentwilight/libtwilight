#pragma once

#include <stdarg.h>

#define MAX_HEAP_RANGES 4

#define PEEK_U16(address) *(volatile unsigned short*)(address)
#define PEEK_U32(address) *(volatile unsigned int*)(address)

#define POKE_U16(address, value) *(volatile unsigned short*)(address) = value
#define POKE_U32(address, value) *(volatile unsigned int*)(address) = value

// TODO: Thread local storage for the first N threads that ask for one (eg. N = 4)
// TODO: Thread pools

struct tw_heap_block {
	struct tw_heap_block *prev; // if the LSB is 1, then this block has been freed
	struct tw_heap_block *next;
};
typedef struct tw_heap_block TwHeapBlockHeader;

struct tw_heap_allocator {
	void *mutex;
	int capacity;
	void *startAddr;
	TwHeapBlockHeader *first;
	TwHeapBlockHeader *last;
	struct tw_heap_allocator *next;
};
typedef struct tw_heap_allocator TwHeapAllocator;

struct tw_stream {
	void *parent;
	int (*transfer)(struct tw_stream *stream, char *data, int size);
};
typedef struct tw_stream TwStream;

typedef struct {
	TwStream stream;
	TwHeapAllocator *alloc;
	int capacity;
	int size;
	char *data;
} TwFlexArray;

typedef struct {
	int capacity;
	int used;
	void **keys;
	unsigned *values;
} TwHashMap;

// These must be defined in the architecture implementation, ie. ppc or arm
extern void *TW_CopyBytes(void *dst, const void *src, int len);
extern unsigned TW_EnableInterrupts(void);
extern unsigned TW_DisableInterrupts(void);
TwHeapAllocator *TW_GetGlobalAllocator(void);

// structures.c
// NOT thread-safe -- many of these functions should be locked on the outside in a concurrent context

TwHeapAllocator TW_MakeHeap(void *startAddress, void *endAddress);
int TW_CalcHeapObjectInnerSize(int count, int elemSize);
int TW_DetermineHeapObjectMaximumSize(TwHeapAllocator *alloc, void *ptr);
void TW_UpdateHeapObjectSize(TwHeapAllocator *alloc, void *ptr, int newSize);
int TW_GetSpaceUntilNextOccupiedHeapObject(TwHeapAllocator *alloc, void *ptr);
void *TW_AllocateHeapObject(TwHeapAllocator *alloc, int count, int elemSize);
void *TW_ReallocateHeapObject(TwHeapAllocator *alloc, void *ptr, int count, int elemSize);
void TW_FreeHeapObject(TwHeapAllocator *alloc, void *ptr);

void *TW_Allocate(TwHeapAllocator *alloc, void *ptr, int count, int elemSize);
void TW_Free(TwHeapAllocator *alloc, void *ptr);

TwFlexArray TW_MakeFlexArray(TwHeapAllocator *alloc, int initialCapacity);
int TW_AppendFlexArray(TwFlexArray *array, char *data, int size);
int TW_ResizeFlexArray(TwFlexArray *array, int newSize);

unsigned TW_GetStringHash(const char *str, int len);
TwHashMap TW_MakeFixedMap(const char **keys, void **key_slots, unsigned *values, int count);
int TW_GetHashMapIndex(TwHashMap *map, const char *key, int len);

// strformat.c
int TW_FormatString(TwStream *sink, int maxOutputSize, const char *str, ...);

// threading.c
// TODO: General concurrent objects
// TODO: General thread pool
int TW_MultiThreadingEnabled(void);
void TW_LockMutex(void **mutex);
void TW_UnlockMutex(void **mutex);
