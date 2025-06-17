#include <twilight.h>

int TW_ParseFat32Partition(TwFile *device, TwPartition outerPartition, TwFlexArray *partitionsOut) {
	return 0;
}

void TW_RegisterFat32Handler(void) {
	TW_RegisterPartitionParser(0x66617433, TW_ParseFat32Partition);
}
