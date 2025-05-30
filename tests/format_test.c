#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/twilight.h"

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

	unsigned long long smallDouble = (1ull << 52) | 1ull;
	unsigned long long bigDouble = (2047ull << 52) - 1ull;

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
	TEST(__LINE__, "Hello %s! Nice to meet you.", "there");
	TEST(__LINE__, "%f", 1.0);
	TEST(__LINE__, "%f", 1.00001);
	TEST(__LINE__, "%f", 1024.0 * 1024.0 * 100.0);
	TEST(__LINE__, "%f", 1500.0 / 18.0);
	TEST(__LINE__, "%f", 0.28);
	TEST(__LINE__, "%f", 0.0078125);
	TEST(__LINE__, "%f", 0.00390625);
	TEST(__LINE__, "%f", 0.001953125);
	TEST(__LINE__, "%f", 1.024 / 99537.0);
	TEST(__LINE__, "%f", 1024.0 * 1024.0 * 1024.0 * 1000.0);
	TEST(__LINE__, "%f", *(double*)&smallDouble);
	TEST(__LINE__, "%f", *(double*)&bigDouble);
	TEST(__LINE__, "%f", 3.141592653585);
	TEST(__LINE__, "%f", -3.141592653585);

	return 0;
}
