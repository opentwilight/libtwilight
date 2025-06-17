#include <twilight.h>

int TW_ParseMbrPartitions(TwFile *device, TwPartition outerPartition, TwFlexArray *partitionsOut) {
	int res = device->seek(device, outerPartition.offsetBytes + 0x1be, TW_SEEK_SET);
    if (res < 0)
        return -1;

	char buf[0x42];
    res = device->read(device, buf, 0x42);
    if (res < 0x42 || buf[0x40] != 0x55 || buf[0x41] != 0xaa)
        return -2;

	TwPartition first_four[4];

    for (int i = 0; i < 4; i++) {
    	unsigned char *offSize = (unsigned char*)&buf[i*16+8];
    	unsigned lba  = (unsigned)offSize[0] | ((unsigned)offSize[1] << 8) | ((unsigned)offSize[2] << 16) | ((unsigned)offSize[3] << 24);
    	unsigned nsec = (unsigned)offSize[4] | ((unsigned)offSize[5] << 8) | ((unsigned)offSize[6] << 16) | ((unsigned)offSize[7] << 24);
        first_four[i].offsetBytes = (long long)lba * 512LL;
        first_four[i].sizeBytes = (long long)nsec * 512LL;
    }

	return 0; // 4;
}

void TW_RegisterMbrHandler(void) {
	TW_RegisterPartitionParser(0x6d627200, TW_ParseMbrPartitions);
}
