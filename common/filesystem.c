#include "twilight.h"

static TwFlexArray _g_mount_name_table = {}; // string pool
static TwFlexArray _g_mount_fs_table = {}; // array of TwFilesystem

static int _try_parse_mbr(TwFile *device, TwPartition *first_four) {
    int res = device->seek(device, 0x1be, TW_SEEK_SET);
    if (res < 0)
        return -1;

    char buf[0x42];
    res = device->read(device, buf, 0x42);
    if (res < 0x42 || buf[0x40] != 0x55 || buf[0x41] != 0xaa)
        return -2;

    for (int i = 0; i < 4; i++) {
    	unsigned char *offSize = (unsigned char*)&buf[i*16+8];
    	unsigned lba  = (unsigned)offSize[0] | ((unsigned)offSize[1] << 8) | ((unsigned)offSize[2] << 16) | ((unsigned)offSize[3] << 24);
    	unsigned nsec = (unsigned)offSize[4] | ((unsigned)offSize[5] << 8) | ((unsigned)offSize[6] << 16) | ((unsigned)offSize[7] << 24);
        first_four[i].offsetBytes = (long long)lba * 512LL;
        first_four[i].sizeBytes = (long long)nsec * 512LL;
    }

	return 4;
}

int TW_EnumeratePartitions(TwFile *device, TwStream *output) {
    TwPartition first_four[4];
    int n_partitions = _try_parse_mbr(device, first_four);
    return 0;
}

TwFilesystem TW_DetermineFilesystem(TwFile *device, TwPartition partition) {
	TwFilesystem fs = {};
	return fs;
}

int TW_MountFilesystem(TwFilesystem *fs, const char *path, int pathLen) {
	char nullChar = 0;
	int size;
	for (size = 0; size < pathLen && path[size] != 0; size++);
	if (size == 0)
		return -1;

	TW_AppendFlexArray(&_g_mount_name_table, path, size);
	TW_AppendFlexArray(&_g_mount_name_table, &nullChar, 1);

	TW_AppendFlexArray(&_g_mount_fs_table, (const char*)fs, sizeof(TwFilesystem));
	return 0;
}

TwFilesystem TW_MountFirstFilesystem(TwFile *device, const char *path, int pathLen) {
	TwFilesystem fs = {};
	return fs;
}

int TW_UnmountFilesystem(const char *path, int pathLen) {
	return 0;
}

// TODO: Fix this function!!! This doesn't actually resolve mount paths correctly.
// It needs to pay attention to slashes to know which directory its pointing at,
//  then return the closest ancestor that is in the mount table (if there is one).
// Right now, it instead just returns the best match from the start of the path.
TwFilesystem TW_ResolveFilesystemPath(const char *path, int pathLen, int *rootCharOffsetOut) {
	int highestLength = 0;
	int highestIndex = -1;
	int idx = 0;
	for (int i = 0; i < _g_mount_name_table.size; i++) {
		int j;
		for (j = 0; j < pathLen && i + j < _g_mount_name_table.size; j++) {
			if (_g_mount_name_table.data[i+j] != path[j] || path[j] == 0)
				break;
		}
		if (j > highestLength) {
			highestLength = j;
			highestIndex = idx;
		}
		idx++;
		i += j;
		for (; i < _g_mount_name_table.size && _g_mount_name_table.data[i] != 0; i++);
		for (; i < _g_mount_name_table.size && _g_mount_name_table.data[i] == 0; i++);
	}

	if (rootCharOffsetOut)
		*rootCharOffsetOut = highestLength;

	TwFilesystem fs = {};

	if (highestIndex >= 0 && highestIndex < _g_mount_fs_table.size)
		fs = ((TwFilesystem*)_g_mount_fs_table.data)[highestIndex];

	return fs;
}

int TW_ListDirectory(unsigned flags, const char *path, TwStream *output) {
	return 0;
}

int TW_CreateDirectory(unsigned flags, const char *path) {
	return 0;
}

int TW_DeleteDirectory(unsigned flags, const char *path) {
	return 0;
}

int TW_RenameDirectory(unsigned flags, const char *oldPath, int oldPathLen, const char *newPath, int newPathLen) {
	return 0;
}

TwFile TW_OpenFile(unsigned flags, const char *path) {
	int len;
	for (len = 0; path[len]; len++);
	int fsRootOff = 0;
	TwFilesystem fs = TW_ResolveFilesystemPath(path, len, &fsRootOff);

	if (fs.openFile) {
		return fs.openFile(&fs, flags, &path[fsRootOff], len - fsRootOff);
	}
	TwFile empty = {};
	return empty;
}

TwFile TW_CreateFile(unsigned flags, long long initialSize, const char *path) {
	TwFile empty = {};
	return empty;
}

int TW_DeleteFile(unsigned flags, const char *path) {
	return 0;
}

int TW_ResizeFile(unsigned flags, long long newSize, const char *path) {
	return 0;
}

int TW_RenameFile(unsigned flags, const char *oldPath, int oldPathLen, const char *newPath, int newPathLen) {
	return 0;
}

int TW_WriteMatchingPaths(const char **potentialPaths, int count, const char *path, int pathLen, TwStream *output) {
	int endsWithSlash = path[pathLen-1] == '/';
	int nMatches = 0;

	for (int i = 0; i < count; i++) {
		int startMatches = 1;
		int j;
		for (j = 0; potentialPaths[i][j]; j++) {
			if (j < pathLen && potentialPaths[i][j] != path[j]) {
				startMatches = 0;
				break;
			}
		}
		if (startMatches && (
			j == pathLen || (
				j > pathLen && (
					endsWithSlash ||
					potentialPaths[i][pathLen] == '/'
				)
			)
		)) {
			nMatches++;
			if (output->transfer(output, (char*)potentialPaths[i], j) == 0) {
				return nMatches;
			}
		}
	}

	return nMatches;
}
