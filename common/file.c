#include <twilight_common.h>

static TwFileProperties _defaultGetProperties(TwFile *file) {
	TwFileProperties empty = {0};
	return empty;
}
static int _defaultTransfer(TwStream *stream, char *buf, int size) {
	return 0;
}
static long long _defaultSeek(TwFile *file, int type, long long seekAmount) {
	return 0;
}
static int _defaultFlush(TwFile *file) {
	return 0;
}
static int _defaultClose(TwFile *file) {
	return 0;
}

TwFile TW_MakeStdin(int (*read)(TwStream*, char*, int)) {
	TwFile file = (TwFile) {
		.getProperties = _defaultGetProperties,
		.seek = _defaultSeek,
		.flush = _defaultFlush,
		.close = _defaultClose,
	};
	file.streamRead = (TwStream) {
		.parent = &file,
		.transfer = read,
	};
	file.streamWrite = (TwStream) {
		.parent = &file,
		.transfer = _defaultTransfer,
	};
	return file;
}

TwFile TW_MakeStdout(int (*write)(TwStream*, char*, int)) {
	TwFile file = (TwFile) {
		.getProperties = _defaultGetProperties,
		.seek = _defaultSeek,
		.flush = _defaultFlush, // Maybe add a non-dummy flush method
		.close = _defaultClose,
	};
	file.streamRead = (TwStream) {
		.parent = &file,
		.transfer = _defaultTransfer,
	};
	file.streamWrite = (TwStream) {
		.parent = &file,
		.transfer = write,
	};
	return file;
}

TwFile *TW_GetFile(int fd) {
	if (fd < 0)
		return (void*)0;

	TwFileBucket *table = TW_GetFileTable();
	while (table && fd >= TW_FILES_PER_BUCKET) {
		fd -= TW_FILES_PER_BUCKET;
		table = table->next;
	}
	return table ? &table->file[fd] : (void*)0;
}
