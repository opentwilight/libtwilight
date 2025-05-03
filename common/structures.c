#include "twilight_common.h"

// NOT thread-safe: many of these functions should be locked on the outside in a concurrent context

TwHeapAllocator TW_MakeHeap(void *startAddress, void *endAddress) {
	int capacity = endAddress - startAddress;
	if (capacity < 0) {
		void *temp = startAddress;
		startAddress = endAddress;
		endAddress = temp;
		capacity = -capacity;
	}
	TwHeapAllocator alloc = (TwHeapAllocator) {
		.capacity = capacity,
		.startAddr = startAddress,
		.first = (TwHeapBlockHeader*)startAddress,
		.last = (TwHeapBlockHeader*)0,
	};
	return alloc;
}

int TW_CalcHeapObjectInnerSize(int count, int elemSize) {
	if (count <= 0)
		return 0;
	else if (elemSize == 0)
		return count;
	else if (elemSize < 0)
		return count * -elemSize;
	return count * elemSize;
}

// TODO: Look at previous heap entries for until there's enough space for the new entry,
//        but look past the first free entry in case there's contiguous free blocks that take up enough space together

void *TW_AllocateHeapObject(TwHeapAllocator *alloc, int count, int elemSize) {
	int size = TW_CalcHeapObjectInnerSize(count, elemSize);
	if (size <= 0)
		return (void*)0;

	TwHeapBlockHeader *hdr = alloc->last ? alloc->last->next : alloc->first;
	unsigned start = (unsigned)&hdr[1];

	int alloc_size = (size + 3) & ~3;
	unsigned next = start + alloc_size;
	if (next + sizeof(TwHeapBlockHeader) > (unsigned)alloc->startAddr + alloc->capacity)
		return (void*)0;

	hdr->next = (TwHeapBlockHeader*)next;
	hdr->prev = alloc->last;
	alloc->last = hdr;

	return (void*)start;
}

void TW_FreeHeapObject(TwHeapAllocator *alloc, void *ptr) {
	TwHeapBlockHeader *obj = &((TwHeapBlockHeader*)ptr)[-1];
	obj->prev = (void*)((unsigned)obj->prev | 1U);
}

// just a dumb hash for now
unsigned TW_GetStringHash(const char *str, int len) {
	unsigned hash = 5381;
	for (int i = 0; i < len || (len == 0 && str[i]); i++) {
		unsigned char c = str[i];
		hash = ((hash << 5) + hash) + c;
	}
	return hash;
}

TwHashMap TW_MakeFixedMap(const char **keys, void **key_slots, unsigned *values, int count) {
	TwHashMap map = (TwHashMap) {
		.capacity = count,
		.used = count,
		.keys = key_slots,
		.values = values
	};

	for (int i = 0; i < count; i++)
		key_slots[i] = (void*)0;

	for (int i = 0; i < count; i++) {
		int idx = (int)(TW_GetStringHash(keys[i], 0) & 0x7FFFffffU) % count;
		for (int j = 0; j < count; j++) {
			void **slot = &key_slots[(idx+j) % count];
			if (*slot == (void*)0) {
				*slot = (void*)keys[i];
				break;
			}
		}
	}

	return map;
}

int TW_GetHashMapIndex(TwHashMap *map, const char *key, int len) {
	if (map->capacity <= 0)
		return -1;

	int idx = (int)(TW_GetStringHash(key, len) & 0x7FFFffffU) % map->capacity;

	for (int i = 0; i < map->capacity; i++) {
		char *k = (char*)map->keys[(idx+i) % map->capacity];
		int j = 0;
		for (; j < len && k[j]; j++) {
			if (k[j] != key[j]) {
				break;
			}
		}
		if (j == len && k[j] == 0) {
			return i;
		}
	}

	return -1;
}
