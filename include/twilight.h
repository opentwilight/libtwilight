#pragma once

#include <stdarg.h>

#define PEEK_U16(address) *(volatile unsigned short*)(address)
#define PEEK_U32(address) *(volatile unsigned int*)(address)

#define POKE_U16(address, value) *(volatile unsigned short*)(address) = value
#define POKE_U32(address, value) *(volatile unsigned int*)(address) = value

#define TW_SEEK_SET 0
#define TW_SEEK_CUR 1
#define TW_SEEK_END 2

#define TW_MODE_NONE  0
#define TW_MODE_READ  1
#define TW_MODE_WRITE 2
#define TW_MODE_RDWR  3

#define TW_FILES_PER_BUCKET 16

#define TW_FILE_TYPE_NONE     0
#define TW_FILE_TYPE_GENERIC  1
#define TW_FILE_TYPE_DISK     2
#define TW_FILE_TYPE_IOS      3

#define TW_FILE_METHOD_NONE   0
#define TW_FILE_METHOD_OPEN   1
#define TW_FILE_METHOD_CLOSE  2
#define TW_FILE_METHOD_READ   3
#define TW_FILE_METHOD_WRITE  4
#define TW_FILE_METHOD_SEEK   5
#define TW_FILE_METHOD_IOCTL  6
#define TW_FILE_METHOD_IOCTLV 7
#define TW_FILE_METHOD_FLUSH  8

// TODO: Thread local storage for the first N threads that ask for one (eg. N = 4)
// TODO: Thread pools

struct tw_mutex;
struct tw_condition_variable;

typedef struct tw_mutex* TwMutex;
typedef struct tw_condition_variable* TwCondition;

struct tw_heap_block {
	struct tw_heap_block *prev; // if the LSB is 1, then this block has been freed
	struct tw_heap_block *next;
};
typedef struct tw_heap_block TwHeapBlockHeader;

struct tw_heap_allocator {
	TwMutex mutex;
	int capacity;
	void *startAddr;
	TwHeapBlockHeader *first;
	TwHeapBlockHeader *last;
	struct tw_heap_allocator *next;
};
typedef struct tw_heap_allocator TwHeapAllocator;

typedef struct {
	void *data;
	unsigned size;
} TwView;

// If you want to include a TwStream in a struct, offsetFromParent must be set to offsetof(your_struct, the_twstream_member)
typedef struct tw_stream TwStream;
struct tw_stream {
	int (*transfer)(struct tw_stream *stream, char *data, int size);
	int offsetFromParent;
};

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

struct _tw_slab_bucket_256 {
	unsigned usedBitField; // book-keeping, so we know which slots have been used
	void *buffer; // so we know what to free
	void *first; // buffer, but aligned to the next 256 bits (32 bytes)
	struct _tw_slab_bucket_256 *next;
};
typedef struct _tw_slab_bucket_256 TwSlabBucket256;

struct _tw_future_st {
	unsigned cvValue;
	TwMutex mtx;
	unsigned result;
	unsigned error;
};
typedef struct _tw_future_st* TwFuture;

TwFuture TW_MakeFuture(unsigned initialResult, unsigned initialError);

struct _tw_queue_st {
	unsigned cvValue;
	TwMutex mtx;
	char *data;
	int capacity;
	int readPos;
	int writePos;
};
typedef struct _tw_queue_st* TwQueue;

typedef struct {
	long long totalSize;
} TwFileProperties;

struct tw_filesystem;

struct tw_file;

typedef void (*TwIoCompletion)(struct tw_file *file, void *userData, int method, long long result);

typedef struct {
	TwIoCompletion handler;
	struct tw_file *file;
	void *userData;
	int method;
} TwIoCompletionContext;

typedef struct {
	struct tw_file *file;
	int method;
	unsigned result;
	unsigned error;
	union {
		struct {
			void *data;
			int size;
		} readWrite;
		struct {
			long long seekAmount;
			int whence;
		} seek;
		struct {
			unsigned method;
			void *input;
			int inputSize;
			void *output;
			int outputSize;
		} ioctl;
		struct {
			unsigned method;
			int nInputs;
			int nOutputs;
			TwView *inputsAndOutputs;
		} ioctlv;
	};
} TwFileTransaction;

struct tw_file {
	struct tw_filesystem *fs;
	unsigned short flags;
	unsigned short type;
	unsigned params[8];

	TwFileProperties (*getProperties)(struct tw_file *file);
	void (*read)(struct tw_file *file, void *userData, void *data, int size, TwIoCompletion completionHandler);
	void (*write)(struct tw_file *file, void *userData, void *data, int size, TwIoCompletion completionHandler);
	void (*seek)(struct tw_file *file, void *userData, long long seekAmount, int whence, TwIoCompletion completionHandler);
	void (*ioctl)(struct tw_file *file, void *userData, unsigned method, void *input, int inputSize, void *output, int outputSize, TwIoCompletion completionHandler);
	void (*ioctlv)(struct tw_file *file, void *userData, unsigned method, int nInputs, int nOutputs, TwView *inputsAndOutputs, TwIoCompletion completionHandler);
	void (*flush)(struct tw_file *file, void *userData, TwIoCompletion completionHandler);
	void (*close)(struct tw_file *file, void *userData, TwIoCompletion completionHandler);
};
typedef struct tw_file TwFile;

typedef struct {
    int next;
    int firstChild;
	unsigned magic;
	unsigned flags;
	long long offsetBytes;
	long long sizeBytes;
} TwPartition;

struct tw_filesystem {
	TwPartition partition;
	TwFile *access;
	int (*listDirectory)(struct tw_filesystem *fs, unsigned flags, const char *path, int pathLen, TwStream *output);
	int (*createDirectory)(struct tw_filesystem *fs, unsigned flags, const char *path, int pathLen);
	int (*deleteDirectory)(struct tw_filesystem *fs, unsigned flags, const char *path, int pathLen);
	int (*renameDirectory)(struct tw_filesystem *fs, unsigned flags, const char *oldPath, int oldPathLen, const char *newPath, int newPathLen);
	TwFile (*openFile)(struct tw_filesystem *fs, unsigned flags, const char *path, int pathLen);
	TwFile (*createFile)(struct tw_filesystem *fs, unsigned flags, long long initialSize, const char *path, int pathLen);
	int (*deleteFile)(struct tw_filesystem *fs, unsigned flags, const char *path, int pathLen);
	int (*resizeFile)(struct tw_filesystem *fs, unsigned flags, long long newSize, const char *path, int pathLen);
	int (*renameFile)(struct tw_filesystem *fs, unsigned flags, const char *oldPath, int oldPathLen, const char *newPath, int newPathLen);
};
typedef struct tw_filesystem TwFilesystem;

// These must be defined in the architecture implementation, ie. ppc or arm
extern void TW_Exit(void);
extern unsigned long long TW_GetCpuTimeBase(void);
extern int TW_CompareAndSwapAtomic(unsigned *address, unsigned expectedMask, unsigned expectedValue, unsigned newValue);
extern unsigned TW_GetAndSetAtomic(unsigned *address, unsigned newValue);
extern unsigned TW_AddAtomic(unsigned *address, int delta);
extern unsigned TW_OrAtomic(unsigned *address, unsigned value);
extern unsigned TW_XorAtomic(unsigned *address, unsigned value);
extern unsigned TW_AndAtomic(unsigned *address, unsigned value);
extern unsigned TW_CountLeadingZeros(unsigned value);
extern int TW_CountBits(unsigned value);
extern int TW_HasOneBitSet(unsigned value);
extern void *TW_CopyBytes(void *dst, const void *src, int len);
extern void *TW_FillBytes(void *dst, unsigned char byte, int len);
unsigned TW_DivideU64(unsigned long long *value, unsigned base);
TwHeapAllocator *TW_GetGlobalAllocator(void);
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
int TW_AppendFlexArray(TwFlexArray *array, const char *data, int size);
int TW_ResizeFlexArray(TwFlexArray *array, int newSize);
void TW_FreeFlexArray(TwFlexArray *array);

unsigned TW_GetStringHash(const char *str, int len);
TwHashMap TW_MakeFixedMap(const char **keys, void **keySlots, unsigned *values, int count);
int TW_GetHashMapIndex(TwHashMap *map, const char *key, int len);

TwSlabBucket256 TW_CreateSlabBucket256(char *buffer);
void *TW_AddSb256Item(TwSlabBucket256 *bucket);
int TW_RemoveSb256Item(TwSlabBucket256 *bucket, void *item);

TwFuture TW_CreateFuture(unsigned initialResult, unsigned initialError);
unsigned long long TW_AwaitFuture(TwFuture future);
int TW_AwaitFutureForTheNext(TwFuture future, int timeoutUs, unsigned long long *resultErrorPtr);
int TW_PeekFuture(TwFuture future, unsigned long long *resultErrorPtr);
void TW_ReachFuture(TwFuture future, unsigned value, unsigned error);
void TW_DestroyFuture(TwFuture future); // o_O

TwQueue TW_CreateQueue(void *buf, int capacity);
int TW_PushToQueue(TwQueue q, void *obj, int size, int timeoutUs);
int TW_PullFromQueue(TwQueue q, void *obj, int size, int timeoutUs);
void TW_DestroyQueue(TwQueue q);

// strformat.c
int TW_FormatString(TwStream *sink, int maxOutputSize, const char *str, ...);
int TW_FormatStringV(TwStream *sink, int maxOutputSize, const char *str, va_list args);
int TW_WriteInteger(char *outBuf, int maxSize, int minWidth, unsigned bits, unsigned base, unsigned flags, unsigned long long value);
int TW_WriteDouble(char *outBuf, int maxSize, int minWidth, int precision, int mode, double value);

// threading -- threading_ppc.c or threading_arm.c
void TW_SetupThreading(void);
int TW_StartThread(void *userData, void *(*entry)(void*));
void TW_Sleep(int durationUs);
TwMutex TW_CreateMutex(void);
void TW_LockMutex(TwMutex mtx);
void TW_UnlockMutex(TwMutex mtx);
void TW_DestroyMutex(TwMutex mutex);
TwCondition TW_CreateCondition(void);
int TW_AwaitCondition(TwCondition cv, TwMutex mutex, int timeoutUs);
void TW_BroadcastCondition(TwCondition cv, TwMutex mutex);
void TW_DestroyCondition(TwCondition cv);

// file.c
TwFile *TW_GetFile(int fd);
TwFile *TW_SetFile(int fd, TwFile file);
int TW_AddFile(TwFile file, TwFile **fileOut);
TwFile TW_MakeStdin(void (*read)(TwFile*, void *, void*, int, TwIoCompletion));
TwFile TW_MakeStdout(void (*write)(TwFile*, void *, void*, int, TwIoCompletion));
int TW_ReadFileSync(TwFile *file, void *data, int size);
int TW_WriteFileSync(TwFile *file, void *data, int size);
long long TW_SeekFileSync(TwFile *file, long long seekAmount, int whence);
int TW_IoctlFileSync(TwFile *file, unsigned method, void *input, int inputSize, void *output, int outputSize);
int TW_IoctlvFileSync(TwFile *file, unsigned method, int nInputs, int nOutputs, TwView *inputsAndOutputs);
int TW_FlushFileSync(TwFile *file);
int TW_CloseFileSync(TwFile *file);

// filesystem.c
void TW_RegisterPartitionParser(unsigned tag, int (*partitionParser)(TwFile *device, TwPartition outerPartition, TwFlexArray *partitionsOut));
int TW_EnumeratePartitions(TwFile *device, TwPartition outerPartition, TwFlexArray *partitionsOut);
TwFilesystem TW_DetermineFilesystem(TwFile *device, TwPartition partition);
int TW_MountFilesystem(TwFilesystem *fs, const char *path);
TwFilesystem TW_MountFirstFilesystem(TwFile *device, const char *path);
int TW_UnmountFilesystem(const char *path);
TwFilesystem TW_ResolvePath(const char *path, int pathLen, int *rootCharOffsetOut);
int TW_WriteMatchingPaths(const char **potentialPaths, int count, const char *path, int pathLen, TwStream *output);

int TW_ListDirectory(unsigned flags, const char *path, TwStream *output);
int TW_CreateDirectory(unsigned flags, const char *path);
int TW_DeleteDirectory(unsigned flags, const char *path);
int TW_RenameDirectory(unsigned flags, const char *oldPath, int oldPathLen, const char *newPath, int newPathLen);
TwFile *TW_OpenFileSync(unsigned flags, const char *path);
TwFile *TW_CreateFileSync(unsigned flags, long long initialSize, const char *path);
int TW_DeleteFile(unsigned flags, const char *path);
int TW_ResizeFile(unsigned flags, long long newSize, const char *path);
int TW_RenameFile(unsigned flags, const char *oldPath, int oldPathLen, const char *newPath, int newPathLen);

int TW_ParseMbrPartitions(TwFile *device, TwPartition outerPartition, TwFlexArray *partitionsOut);
void TW_RegisterMbrHandler(void);
int TW_ParseFat32Partition(TwFile *device, TwPartition outerPartition, TwFlexArray *partitionsOut);
void TW_RegisterFat32Handler(void);
