#include <twilight.h>

struct _tw_file_bucket {
	TwFile file[TW_FILES_PER_BUCKET];
	struct _tw_file_bucket *next;
} _g_file_table = {0};

static TwFileProperties _defaultGetProperties(TwFile *file) {
	TwFileProperties empty = {0};
	return empty;
}
static void _defaultRead(TwFile *file, void *userData, void *buf, int size, TwIoCompletion completionHandler) {
	completionHandler(file, userData, TW_FILE_METHOD_READ, 0);
}
static void _defaultWrite(TwFile *file, void *userData, void *buf, int size, TwIoCompletion completionHandler) {
	completionHandler(file, userData, TW_FILE_METHOD_WRITE, 0);
}
static void _defaultSeek(TwFile *file, void *userData, long long seekAmount, int whence, TwIoCompletion completionHandler) {
	completionHandler(file, userData, TW_FILE_METHOD_SEEK, 0);
}
static void _defaultFlush(TwFile *file, void *userData, TwIoCompletion completionHandler) {
	completionHandler(file, userData, TW_FILE_METHOD_FLUSH, 0);
}
static void _defaultClose(TwFile *file, void *userData, TwIoCompletion completionHandler) {
	completionHandler(file, userData, TW_FILE_METHOD_CLOSE, 0);
}

TwFile TW_MakeStdin(void (*read)(TwFile*, void *, void*, int, TwIoCompletion)) {
	TwFile file = (TwFile) {
		.getProperties = _defaultGetProperties,
		.seek = _defaultSeek,
		.flush = _defaultFlush,
		.close = _defaultClose,
	};
	file.read = read;
	file.write = _defaultWrite;
	return file;
}

TwFile TW_MakeStdout(void (*write)(TwFile*, void*, void*, int, TwIoCompletion)) {
	TwFile file = (TwFile) {
		.getProperties = _defaultGetProperties,
		.seek = _defaultSeek,
		.flush = _defaultFlush, // Maybe add a non-dummy flush method
		.close = _defaultClose,
	};
	file.read = _defaultRead;
	file.write = write;
	return file;
}

TwFile *TW_SetFile(int fd, TwFile file) {
	if (fd < 0)
		return (TwFile*)0;

	struct _tw_file_bucket *table = &_g_file_table;
	while (table && fd >= TW_FILES_PER_BUCKET) {
		fd -= TW_FILES_PER_BUCKET;
		table = table->next;
	}
	if (table) {
		TwFile *obj = &table->file[fd];
		*obj = file;
		if (!obj->type)
			obj->type = TW_FILE_TYPE_GENERIC;
		return obj;
	}
	return (TwFile*)0;
}

TwFile *TW_GetFile(int fd) {
	if (fd < 0)
		return (void*)0;

	struct _tw_file_bucket *table = &_g_file_table;
	while (table && fd >= TW_FILES_PER_BUCKET) {
		fd -= TW_FILES_PER_BUCKET;
		table = table->next;
	}
	return table ? &table->file[fd] : (void*)0;
}

int TW_AddFile(TwFile file, TwFile **fileOut) {
	int fd = 3;
	int fdOffset = 0;
	struct _tw_file_bucket *table = &_g_file_table;
	while (table) {
		for (; fd - fdOffset < TW_FILES_PER_BUCKET; fd++) {
			TwFile *obj = &table->file[fd - fdOffset];
			if (!obj->type) {
				*obj = file;
				if (!obj->type)
					obj->type = TW_FILE_TYPE_GENERIC;
				if (fileOut)
					*fileOut = obj;
				return fd;
			}
		}
		fdOffset += TW_FILES_PER_BUCKET;
		table = table->next;
	}
	if (fileOut)
		*fileOut = (TwFile*)0;
	return -1;
}

static void _tw_sync_io_completion_handler(struct tw_file *file, void *userData, int method, long long result) {
	TwFuture f = (TwFuture)userData;
	TW_ReachFuture(f, (unsigned)(result & 0xFFFFffffLL), (unsigned)((result >> 32) & 0xFFFFffffLL));
}

int TW_ReadFileSync(TwFile *file, void *data, int size) {
	TwFuture f = TW_CreateFuture(0, 0);
	file->read(file, f, data, size, _tw_sync_io_completion_handler);
	unsigned long long resultError = TW_AwaitFuture(f);
	TW_DestroyFuture(f);
	return (int)(resultError & 0xFFFFffff);
}

int TW_WriteFileSync(TwFile *file, void *data, int size) {
	TwFuture f = TW_CreateFuture(0, 0);
	file->write(file, f, data, size, _tw_sync_io_completion_handler);
	unsigned long long resultError = TW_AwaitFuture(f);
	TW_DestroyFuture(f);
	return (int)(resultError & 0xFFFFffff);
}

long long TW_SeekFileSync(TwFile *file, long long seekAmount, int whence) {
	TwFuture f = TW_CreateFuture(0, 0);
	file->seek(file, f, seekAmount, whence, _tw_sync_io_completion_handler);
	unsigned long long resultError = TW_AwaitFuture(f);
	TW_DestroyFuture(f);
	return (long long)resultError;
}

int TW_IoctlFileSync(TwFile *file, unsigned method, void *input, int inputSize, void *output, int outputSize) {
	TwFuture f = TW_CreateFuture(0, 0);
	file->ioctl(file, f, method, input, inputSize, output, outputSize, _tw_sync_io_completion_handler);
	unsigned long long resultError = TW_AwaitFuture(f);
	TW_DestroyFuture(f);
	return (int)(resultError & 0xFFFFffff);
}

int TW_IoctlvFileSync(TwFile *file, unsigned method, int nInputs, int nOutputs, TwView *inputsAndOutputs) {
	TwFuture f = TW_CreateFuture(0, 0);
	file->ioctlv(file, f, method, nInputs, nOutputs, inputsAndOutputs, _tw_sync_io_completion_handler);
	unsigned long long resultError = TW_AwaitFuture(f);
	TW_DestroyFuture(f);
	return (int)(resultError & 0xFFFFffff);
}

int TW_FlushFileSync(TwFile *file) {
	TwFuture f = TW_CreateFuture(0, 0);
	file->flush(file, f, _tw_sync_io_completion_handler);
	unsigned long long resultError = TW_AwaitFuture(f);
	TW_DestroyFuture(f);
	return (int)(resultError & 0xFFFFffff);
}

int TW_CloseFileSync(TwFile *file) {
	TwFuture f = TW_CreateFuture(0, 0);
	file->close(file, f, _tw_sync_io_completion_handler);
	unsigned long long resultError = TW_AwaitFuture(f);
	TW_DestroyFuture(f);
	return (int)(resultError & 0xFFFFffff);
}
