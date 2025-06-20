#include <twilight.h>

struct _tw_file_bucket {
	TwFile file[TW_FILES_PER_BUCKET];
	struct _tw_file_bucket *next;
} _g_file_table = {0};

static TwFileProperties _defaultGetProperties(TwFile *file) {
	TwFileProperties empty = {0};
	return empty;
}
static int _defaultTransfer(TwFile *file, void *buf, int size) {
	return 0;
}
static long long _defaultSeek(TwFile *file, long long seekAmount, int whence) {
	return 0;
}
static int _defaultFlush(TwFile *file) {
	return 0;
}
static int _defaultClose(TwFile *file) {
	return 0;
}

TwFile TW_MakeStdin(int (*read)(TwFile*, void*, int)) {
	TwFile file = (TwFile) {
		.getProperties = _defaultGetProperties,
		.seek = _defaultSeek,
		.flush = _defaultFlush,
		.close = _defaultClose,
	};
	file.read = read;
	file.write = _defaultTransfer;
	return file;
}

TwFile TW_MakeStdout(int (*write)(TwFile*, void*, int)) {
	TwFile file = (TwFile) {
		.getProperties = _defaultGetProperties,
		.seek = _defaultSeek,
		.flush = _defaultFlush, // Maybe add a non-dummy flush method
		.close = _defaultClose,
	};
	file.read = _defaultTransfer;
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
