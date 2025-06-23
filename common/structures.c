#include <twilight.h>

#define BEGIN_ALLOCATOR_METHOD() \
	if (!alloc) { \
		alloc = TW_GetGlobalAllocator(); \
	} \
	if (alloc->mutex) { \
		TW_LockMutex(alloc->mutex); \
	}

#define END_ALLOCATOR_METHOD() \
	if (alloc->mutex) { \
		TW_UnlockMutex(alloc->mutex); \
	}

TwHeapAllocator TW_MakeHeapAllocator(void *startAddress, void *endAddress) {
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

static void *_tw_search_for_suitable_block(TwHeapAllocator *alloc, int allocSize) {
	TwHeapBlockHeader *hdr = alloc->first;
	while (hdr) {
		if (((unsigned)hdr->prev & 1) != 0) {
			int blockSize = TW_DetermineHeapObjectMaximumSize(alloc, &hdr[1]);
			if (blockSize >= allocSize)
				return &hdr[1];
		}
		hdr = hdr->next;
	}
	return (void*)0;
}

static void *_tw_allocate_heap_object(TwHeapAllocator *alloc, int count, int elemSize) {
	int size = TW_CalcHeapObjectInnerSize(count, elemSize);
	if (size <= 0)
		return (void*)0;

	while (alloc) {
		TwHeapBlockHeader *hdr = alloc->last ? alloc->last->next : alloc->first;
		unsigned start = (unsigned)&hdr[1];

		int alloc_size = (size + 3) & ~3;
		unsigned next = start + alloc_size;

		if (next + sizeof(TwHeapBlockHeader) > (unsigned)alloc->startAddr + alloc->capacity) {
			void *ptr = _tw_search_for_suitable_block(alloc, alloc_size);
			start = (unsigned)ptr;
		}

		if (start) {
			hdr->next = (TwHeapBlockHeader*)next;
			hdr->prev = alloc->last;
			alloc->last = hdr;
			return (void*)start;
		}

		alloc = alloc->next;
	}
	return (void*)0;
}

static void _tw_free_heap_object(TwHeapAllocator *alloc, void *ptr) {
	while (alloc) {
		if (ptr >= alloc->startAddr && ptr < (void*)((char*)alloc->startAddr + alloc->capacity)) {
			TwHeapBlockHeader *obj = &((TwHeapBlockHeader*)ptr)[-1];
			obj->prev = (void*)((unsigned)obj->prev | 1U);
			break;
		}
		alloc = alloc->next;
	}
}

static void *_tw_reallocate_heap_object(TwHeapAllocator *alloc, void *ptr, int count, int elemSize) {
	int cur_size = TW_DetermineHeapObjectMaximumSize(alloc, ptr);
	int new_size = TW_CalcHeapObjectInnerSize(count, elemSize);
	if (new_size <= cur_size)
		return ptr;

	void *new_ptr = _tw_allocate_heap_object(alloc, count, elemSize);
	if (!new_ptr)
		return (void*)0;

	TW_CopyBytes(new_ptr, ptr, cur_size);
	_tw_free_heap_object(alloc, ptr);
	return new_ptr;
}

void *TW_AllocateHeapObject(TwHeapAllocator *alloc, int count, int elemSize) {
	BEGIN_ALLOCATOR_METHOD();
	void *ptr = _tw_allocate_heap_object(alloc, count, elemSize);
	END_ALLOCATOR_METHOD();
	return ptr;
}

void *TW_ReallocateHeapObject(TwHeapAllocator *alloc, void *ptr, int count, int elemSize) {
	BEGIN_ALLOCATOR_METHOD();
	void *new_ptr = _tw_reallocate_heap_object(alloc, ptr, count, elemSize);
	END_ALLOCATOR_METHOD();
	return new_ptr;
}

void TW_FreeHeapObject(TwHeapAllocator *alloc, void *ptr) {
	BEGIN_ALLOCATOR_METHOD();
	_tw_free_heap_object(alloc, ptr);
	END_ALLOCATOR_METHOD();
}

void *TW_Allocate(TwHeapAllocator *alloc, void *ptr, int count, int elemSize) {
	if (!alloc)
		alloc = TW_GetGlobalAllocator();
	if (ptr)
		return TW_ReallocateHeapObject(alloc, ptr, count, elemSize);
	return TW_AllocateHeapObject(alloc, count, elemSize);
}

void TW_Free(TwHeapAllocator *alloc, void *ptr) {
	if (!alloc)
		alloc = TW_GetGlobalAllocator();
	TW_FreeHeapObject(alloc, ptr);
}

int TW_DetermineHeapObjectMaximumSize(TwHeapAllocator *alloc, void *ptr) {
	int total_inner = 0;
	TwHeapBlockHeader *first_hdr = (TwHeapBlockHeader*)ptr - 1;
	TwHeapBlockHeader *hdr = first_hdr;

	while (1) {
		if (hdr->next == (void*)0 || hdr == alloc->last)
			return (unsigned)alloc->startAddr + alloc->capacity - (unsigned)ptr - sizeof(TwHeapBlockHeader);

		if (((unsigned)hdr->next->prev & 1) == 0)
			return (unsigned)hdr - (unsigned)ptr;

		TwHeapBlockHeader *next = hdr->next;
		first_hdr->next = next->next;
		next->prev = (void*)0;
		next->next = (void*)0;
		hdr = next;
	}

	return 0;
}

static int _tw_write_buffer_stream(TwStream *stream, char *data, int size) {
	TwBufferStream *bs = (TwBufferStream*)((unsigned)stream - stream->offsetFromParent);
	if (bs->offset + size > bs->capacity)
		return -1;

	TW_CopyBytes(&bs->buffer[bs->offset], data, size);
	bs->offset += size;
	return size;
}

TwBufferStream TW_MakeBufferStream(char *data, int size) {
	TwBufferStream bs = (TwBufferStream) {
		.buffer = data,
		.offset = 0,
		.capacity = size
	};
	bs.stream = (TwStream) {
		.transfer = _tw_write_buffer_stream,
		.offsetFromParent = 0,
	};
	return bs;
}

static int _tw_write_flex_array(TwStream *stream, char *data, int size) {
	return TW_AppendFlexArray((TwFlexArray*)((unsigned)stream - stream->offsetFromParent), data, size);
}

TwFlexArray TW_MakeFlexArray(TwHeapAllocator *alloc, int initialCapacity) {
	int capacity = 0;
	char *ptr = (void*)0;
	if (initialCapacity > 0) {
		capacity = initialCapacity;
		ptr = TW_AllocateHeapObject(alloc, capacity, 1);
	}
	TwFlexArray array = (TwFlexArray) {
		.alloc = alloc,
		.capacity = capacity,
		.size = 0,
		.data = ptr
	};
	array.stream = (TwStream) {
		.transfer = _tw_write_flex_array,
		.offsetFromParent = 0,
	};
	return array;
}

int TW_AppendFlexArray(TwFlexArray *array, const char *data, int size) {
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
		array->data = TW_Allocate(array->alloc, array->data, newSize, 1);
		// allocation failure
		if (!array->data) {
			return -1;
		}
		array->capacity = new_cap;
	}

	array->size = newSize;
	return newSize;
}

void TW_FreeFlexArray(TwFlexArray *array) {
	if (array->data) {
		TW_Free(array->alloc, array->data);
		array->data = (void*)0;
	}
	array->capacity = 0;
	array->size = 0;
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

// buffer must be 32*32 + misalignment bytes long
// if you don't know the alignment, assume misalignment = 32
// so, 1056 bytes.
TwSlabBucket256 TW_CreateSlabBucket256(char *buffer) {
	char *ptr = (char*)(((unsigned)buffer) + 0x1f) & ~0x1f);
	TwSlabBucket256 bucket = {
		.usedBitField = 0,
		.buffer = buffer,
		.first = ptr,
		.next = (void*)0,
	};
	return bucket;
}

void *TW_AddSb256Item(TwSlabBucket256 *bucket) {
	int flag = 32;
	while (1) {
		unsigned unused = ~bucket->usedBitField;
		flag = TW_CountLeadingZeroes(unused);
		if (flag < 32)
			break;

		if (!bucket->next) {
			void *buffer = TW_Allocate((void*)0, (void*)0, 1056 + sizeof(TwSlabBucket256), 1);
			TwSlabBucket256 *hdr = (TwSlabBucket256*)((char*)buffer + 1056);
			*hdr = TW_CreateSlabBucket256(buffer);
			bucket->next = hdr;
			flag = 0;
			break;
		}

		bucket = bucket->next;
	}

	int pos = 31 - flag;
	bucket->usedBitField |= 1u << pos;
	return (char*)bucket->first + (pos << 5);
}

int TW_RemoveSb256Item(TwSlabBucket256 *bucket, void *item) {
	unsigned itemAddr = (unsigned)item;
	while (bucket) {
		unsigned firstAddr = (unsigned)bucket->first;
		if (itemAddr >= firstAddr && itemAddr < firstAddr + 1024) {
			int pos = (itemAddr - firstAddr) >> 5;
			bucket->usedBitField &= ~(1 << pos);
			return 1;
		}
		bucket = bucket->next;
	}
	return 0;
}

void TW_AwaitFuture(TwFuture *future, int timeoutMs) {
	
}

void TW_CompleteFuture(TwFuture *future, void *value) {
	
}

void TW_FailFuture(TwFuture *future, int res) {
	
}

void TW_ReachFuture(TwFuture *future, void *value, int res) {
	
}
