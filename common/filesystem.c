#include "twilight.h"

static TwFlexArray _g_mount_name_table; // string pool
static TwFlexArray _g_mount_fs_table; // array of TwFilesystem

static int _try_parse_mbr(TwFile *device, TwPartition *first_four) {
    int res = device->seek(device, 0x1be);
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

TwStream TW_EnumeratePartitions(TwFile *device) {
    TwPartition first_four[4];
    int n_partitions = _try_parse_mbr(first_four, 4);
}

TwFilesystem TW_DetermineFilesystem(TwFile *device, TwPartition partition) {

}

int TW_MountFilesystem(TwFilesystem *fs, const char *path, int pathLen) {

}

TwFilesystem TW_MountFirstFilesystem(TwFile *device, const char *path, int pathLen) {

}

int TW_UnmountFilesystem(const char *path, int pathLen) {

}

TwFilesystem TW_ResolvePath(const char *path, int pathLen, int *rootCharOffset) {
    
}
