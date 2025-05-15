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

void *TW_ReallocateHeapObject(TwHeapAllocator *alloc, void *ptr, int count, int elemSize) {
	int cur_size = TW_RetrieveHeapObjectInnerSize(alloc, ptr);
	int new_size = TW_CalcHeapObjectInnerSize(count, elemSize);
	if (new_size <= cur_size) {
		TW_UpdateHeapObjectSize(alloc, ptr, new_size);
		return ptr;
	}

	int space_until_next = TW_GetSpaceUntilNextOccupiedHeapObject(alloc, ptr);
	if (space_until_next < 0 || new_size < space_until_next) {
		TW_UpdateHeapObjectSize(alloc, ptr, new_size);
		return ptr;
	}

	void *new_ptr = TW_AllocateHeapObject(alloc, count, elemSize);
	if (!new_ptr)
		return (void*)0;

	TW_CopyBytes(new_ptr, ptr, cur_size);
	TW_FreeHeapObject(alloc, ptr);
	return new_ptr;
}

void TW_FreeHeapObject(TwHeapAllocator *alloc, void *ptr) {
	TwHeapBlockHeader *obj = &((TwHeapBlockHeader*)ptr)[-1];
	obj->prev = (void*)((unsigned)obj->prev | 1U);
}

// TODO !!!

int TW_RetrieveHeapObjectInnerSize(TwHeapAllocator *alloc, void *ptr) {
	return 0;
}

void TW_UpdateHeapObjectSize(TwHeapAllocator *alloc, void *ptr, int newSize) {

}

int TW_GetSpaceUntilNextOccupiedHeapObject(TwHeapAllocator *alloc, void *ptr) {
	return 0;
}

static int _tw_write_flex_array(TwStream *stream, char *data, int size) {
	return TW_AppendFlexArray((TwFlexArray*)stream->parent, data, size);
}

TwFlexArray TW_MakeFlexArray(TwHeapAllocator *alloc, int initialCapacity) {
	int capacity = 0;
	char *ptr = (void*)0;
	if (initialCapacity > 0) {
		capacity = initialCapacity;
		if (alloc)
			ptr = TW_AllocateHeapObject(alloc, capacity, 1);
		else
			ptr = TW_AllocateGlobal(capacity, 1);
	}
	TwFlexArray array = (TwFlexArray) {
		.alloc = alloc,
		.capacity = capacity,
		.size = 0,
		.data = ptr
	};
	array.stream = (TwStream) {
		.parent = &array,
		.transfer = _tw_write_flex_array
	};
	return array;
}

int TW_AppendFlexArray(TwFlexArray *array, char *data, int size) {
	int pos = array->size;
	if (size <= 0)
		return pos;

	int new_size = TW_ResizeFlexArray(array, pos + size);
	if (new_size != pos + size)
		return new_size;

	TW_CopyBytes(&array->data[pos], data, size);
	return new_size;
}

int TW_ResizeFlexArray(TwFlexArray *array, int newSize) {
	if (newSize <= array->size) {
		if (newSize < 0)
			newSize = 0;
		return newSize;
	}

	int new_cap = array->capacity;
	if (new_cap < 64)
		new_cap = 64;
	while (newSize > new_cap)
		new_cap = ((new_cap + 1) << 4) / 10;

	if (new_cap > array->capacity) {
		if (array->data) {
			if (array->alloc)
				array->data = TW_ReallocateHeapObject(array->alloc, array->data, newSize, 1);
			else
				array->data = TW_ReallocateGlobal(array->data, newSize, 1);
		}
		else {
			if (array->alloc)
				array->data = TW_AllocateHeapObject(array->alloc, newSize, 1);
			else
				array->data = TW_AllocateGlobal(newSize, 1);
		}
		// allocation failure
		if (!array->data) {
			return -1;
		}
		array->capacity = new_cap;
	}

	array->size = newSize;
	return newSize;
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

TwHashMap TW_MakeFixedMap(const char **keys, void **keySlots, unsigned *values, int count) {
	TwHashMap map = (TwHashMap) {
		.capacity = count,
		.used = count,
		.keys = keySlots,
		.values = values
	};

	for (int i = 0; i < count; i++)
		keySlots[i] = (void*)0;

	for (int i = 0; i < count; i++) {
		int idx = (int)(TW_GetStringHash(keys[i], 0) & 0x7FFFffffU) % count;
		for (int j = 0; j < count; j++) {
			void **slot = &keySlots[(idx+j) % count];
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
