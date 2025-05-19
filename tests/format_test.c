#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/twilight_common.h"

extern void initTestingGlobalAllocator();

#define BUFFER_SIZE 1024

#define TEST(lineno, fmt, ...) \
	bs.offset = 0; \
	TW_FormatString(&bs.stream, BUFFER_SIZE, fmt, __VA_ARGS__); \
	snprintf(bufstd, BUFFER_SIZE, fmt, __VA_ARGS__); \
	if (strcmp(buftw, bufstd)) { \
		printf("Format test on line %d failed!\nTW_FormatString: %s\nsnprintf: %s\n", lineno, buftw, bufstd); \
		return 1; \
	}

int main() {
	initTestingGlobalAllocator();

	char buftw[BUFFER_SIZE];
	char bufstd[BUFFER_SIZE];

	TwBufferStream bs = TW_MakeBufferStream(buftw, BUFFER_SIZE);

	TEST(__LINE__, "%d", 5)

	return 0;
}
