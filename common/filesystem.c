#include "twilight.h"

static TwFlexArray _g_mount_name_table; // string pool
static TwFlexArray _g_mount_fs_table; // array of TwFilesystem

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
	return 0;
}

TwFilesystem TW_MountFirstFilesystem(TwFile *device, const char *path, int pathLen) {
	TwFilesystem fs = {};
	return fs;
}

int TW_UnmountFilesystem(const char *path, int pathLen) {
	return 0;
}

TwFilesystem *TW_ResolvePath(const char *path, int pathLen, int *rootCharOffset) {
    return (TwFilesystem*)0;
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
