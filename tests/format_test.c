#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/twilight_common.h"

extern void initTestingGlobalAllocator();

#define BUFFER_SIZE 1024

#define TEST(lineno, fmt, ...) \
	bs.offset = 0; \
	lenA = TW_FormatString(&bs.stream, BUFFER_SIZE, fmt, __VA_ARGS__); \
	lenB = snprintf(bufstd, BUFFER_SIZE, fmt, __VA_ARGS__); \
	if (lenA != lenB || strcmp(buftw, bufstd)) { \
		printf("Format test on line %d failed!\nTW_FormatString:  \"%s\"\nsnprintf:         \"%s\"\n", lineno, buftw, bufstd); \
	}

int main() {
	initTestingGlobalAllocator();

	char buftw[BUFFER_SIZE];
	char bufstd[BUFFER_SIZE];

	TwBufferStream bs = TW_MakeBufferStream(buftw, BUFFER_SIZE);
	int lenA = 0;
	int lenB = 0;

	TEST(__LINE__, "%d", 5)
	TEST(__LINE__, "%d", 26)
	TEST(__LINE__, "%03d", 26)
	TEST(__LINE__, "%d", -26)
	TEST(__LINE__, "%+d", 26)
	TEST(__LINE__, "%+d", -26)
	TEST(__LINE__, "%+04d", 26)
	TEST(__LINE__, "%+04d", -26)
	TEST(__LINE__, "%+-4d", 26)
	TEST(__LINE__, "%+-4d", -26)
	TEST(__LINE__, "%-8d", 26)
	TEST(__LINE__, "%-8d", -26)
	TEST(__LINE__, "%o", 29)
	TEST(__LINE__, "%o", -29)
	TEST(__LINE__, "%#o", 29)
	TEST(__LINE__, "%#o", -29)
	TEST(__LINE__, "%6o", 29)
	TEST(__LINE__, "%6o", -29)
	TEST(__LINE__, "%#6o", 29)
	TEST(__LINE__, "%#6o", -29)
	TEST(__LINE__, "%x", 149)
	TEST(__LINE__, "%x", -149)
	TEST(__LINE__, "%#x", 149)
	TEST(__LINE__, "%#x", -149)
	TEST(__LINE__, "%X", 149)
	TEST(__LINE__, "%X", -149)
	TEST(__LINE__, "%#X", 149)
	TEST(__LINE__, "%#X", -149)
	TEST(__LINE__, "%04x", 149)
	TEST(__LINE__, "%04x", -149)
	TEST(__LINE__, "%#04x", 149)
	TEST(__LINE__, "%#04x", -149)
	TEST(__LINE__, "%04X", 149)
	TEST(__LINE__, "%04X", -149)
	TEST(__LINE__, "%#04X", 149)
	TEST(__LINE__, "%#04X", -149)
	TEST(__LINE__, "a%dc", -26)
	TEST(__LINE__, "%*d", 5, 67);

	return 0;
}
