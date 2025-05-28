#pragma once

#include <stdarg.h>

#define PEEK_U16(address) *(volatile unsigned short*)(address)
#define PEEK_U32(address) *(volatile unsigned int*)(address)

#define POKE_U16(address, value) *(volatile unsigned short*)(address) = value
#define POKE_U32(address, value) *(volatile unsigned int*)(address) = value

#define MAX_HEAP_RANGES 4
#define TW_FILES_PER_BUCKET 16

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
	char *buffer;
	int offset;
	int capacity;
} TwBufferStream;

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

typedef struct {
	long long totalSize;
	int partitionCount;
} TwFileProperties;

struct _tw_filesystem;

struct _tw_file {
	unsigned tag;
	struct _tw_filesystem *fs;
	TwFileProperties (*getProperties)(struct _tw_file *file);
	TwStream streamRead;
	TwStream streamWrite;
	long long (*seek)(struct _tw_file *file, int type, long long seekAmount);
	int (*flush)(struct _tw_file *file);
	int (*close)(struct _tw_file *file);
};
typedef struct _tw_file TwFile;

struct _tw_filesystem {
	void *parent;
	TwFile access;
	TwStream (*listDirectory)(struct _tw_filesystem *fs, unsigned flags, const char *path, int pathLen);
	int (*createDirectory)(struct _tw_filesystem *fs, unsigned flags, const char *path, int pathLen);
	int (*deleteDirectory)(struct _tw_filesystem *fs, unsigned flags, const char *path, int pathLen);
	int (*renameDirectory)(struct _tw_filesystem *fs, unsigned flags, const char *oldPath, int oldPathLen, const char *newPath, int newPathLen);
	TwFile (*openFile)(struct _tw_filesystem *fs, unsigned flags, const char *path, int pathLen);
	TwFile (*createFile)(struct _tw_filesystem *fs, unsigned flags, long long initialSize, const char *path, int pathLen);
	int (*deleteFile)(struct _tw_filesystem *fs, unsigned flags, const char *path, int pathLen);
	int (*resizeFile)(struct _tw_filesystem *fs, unsigned flags, long long newSize, const char *path, int pathLen);
	int (*renameFile)(struct _tw_filesystem *fs, unsigned flags, const char *oldPath, int oldPathLen, const char *newPath, int newPathLen);
};
typedef struct _tw_filesystem TwFilesystem;

typedef struct {
	unsigned magic;
	unsigned flags;
	long long offsetBytes;
	long long sizeBytes;
} TwPartition;

struct _tw_file_bucket {
	TwFile file[TW_FILES_PER_BUCKET];
	struct _tw_file_bucket *next;
};
typedef struct _tw_file_bucket TwFileBucket;

// These must be defined in the architecture implementation, ie. ppc or arm
extern void TW_Exit(void);
extern void *TW_CopyBytes(void *dst, const void *src, int len);
extern void *TW_FillBytes(void *dst, unsigned char byte, int len);
unsigned TW_DivideU64(unsigned long long *value, unsigned base);
extern unsigned TW_EnableInterrupts(void);
extern unsigned TW_DisableInterrupts(void);
TwHeapAllocator *TW_GetGlobalAllocator(void);
TwFileBucket *TW_GetFileTable(void);
int TW_Printf(const char *fmt, ...);

// structures.c
// NOT thread-safe -- many of these functions should be locked on the outside in a concurrent context

TwHeapAllocator TW_MakeHeapAllocator(void *startAddress, void *endAddress);
int TW_CalcHeapObjectInnerSize(int count, int elemSize);
int TW_DetermineHeapObjectMaximumSize(TwHeapAllocator *alloc, void *ptr);
void TW_UpdateHeapObjectSize(TwHeapAllocator *alloc, void *ptr, int newSize);
int TW_GetSpaceUntilNextOccupiedHeapObject(TwHeapAllocator *alloc, void *ptr);
void *TW_AllocateHeapObject(TwHeapAllocator *alloc, int count, int elemSize);
void *TW_ReallocateHeapObject(TwHeapAllocator *alloc, void *ptr, int count, int elemSize);
void TW_FreeHeapObject(TwHeapAllocator *alloc, void *ptr);

void *TW_Allocate(TwHeapAllocator *alloc, void *ptr, int count, int elemSize);
void TW_Free(TwHeapAllocator *alloc, void *ptr);

TwBufferStream TW_MakeBufferStream(char *data, int size);

TwFlexArray TW_MakeFlexArray(TwHeapAllocator *alloc, int initialCapacity);
int TW_AppendFlexArray(TwFlexArray *array, char *data, int size);
int TW_ResizeFlexArray(TwFlexArray *array, int newSize);

unsigned TW_GetStringHash(const char *str, int len);
TwHashMap TW_MakeFixedMap(const char **keys, void **keySlots, unsigned *values, int count);
int TW_GetHashMapIndex(TwHashMap *map, const char *key, int len);

// strformat.c
int TW_FormatString(TwStream *sink, int maxOutputSize, const char *str, ...);
int TW_FormatStringV(TwStream *sink, int maxOutputSize, const char *str, va_list args);
int TW_WriteInteger(char *outBuf, int maxSize, int minWidth, unsigned bits, unsigned base, unsigned flags, unsigned long long value);
int TW_WriteDouble(char *outBuf, int maxSize, int minWidth, int precision, int mode, double value);

// threading.c
// TODO: General concurrent objects
// TODO: General thread pool
int TW_MultiThreadingEnabled(void);
void TW_LockMutex(void **mutex);
void TW_UnlockMutex(void **mutex);

// file.c
TwFile TW_MakeStdin(int (*read)(TwStream*, char*, int));
TwFile TW_MakeStdout(int (*write)(TwStream*, char*, int));

// filesystem.c
TwStream TW_EnumeratePartitions(TwFile *device);
TwFilesystem *TW_MountFilesystem(TwFile *device, TwPartition partition, const char *path, int pathLen);
int TW_UnmountFilesystem(const char *path, int pathLen);

TwStream TW_ListDirectory(unsigned flags, const char *path, int pathLen);
int TW_CreateDirectory(unsigned flags, const char *path, int pathLen);
int TW_DeleteDirectory(unsigned flags, const char *path, int pathLen);
int TW_RenameDirectory(unsigned flags, const char *oldPath, int oldPathLen, const char *newPath, int newPathLen);
TwFile TW_OpenFile(unsigned flags, const char *path, int pathLen);
TwFile TW_CreateFile(unsigned flags, long long initialSize, const char *path, int pathLen);
int TW_DeleteFile(unsigned flags, const char *path, int pathLen);
int TW_ResizeFile(unsigned flags, long long newSize, const char *path, int pathLen);
int TW_RenameFile(unsigned flags, const char *oldPath, int oldPathLen, const char *newPath, int newPathLen);
