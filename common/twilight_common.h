#pragma once

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

typedef struct {
	int capacity;
	void *startAddr;
	TwHeapBlockHeader *first;
	TwHeapBlockHeader *last;
} TwHeapAllocator;

typedef struct {
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

// structures.c -- NOT thread-safe -- many of these functions should be locked on the outside in a concurrent context
TwHeapAllocator TW_MakeHeap(void *startAddress, void *endAddress);
int TW_CalcHeapObjectInnerSize(int count, int elemSize);
void *TW_AllocateHeapObject(TwHeapAllocator *alloc, int count, int elemSize);
void TW_FreeHeapObject(TwHeapAllocator *alloc, void *ptr);

unsigned TW_GetStringHash(const char *str, int len);
TwHashMap TW_MakeFixedMap(const char **keys, void **key_slots, unsigned *values, int count);
int TW_GetHashMapIndex(TwHashMap *map, const char *key, int len);

// TODO: General concurrent objects
// TODO: General thread pool
