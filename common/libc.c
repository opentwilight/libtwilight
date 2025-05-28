#include <twilight.h>

void *memcpy(void *dst, const void *src, unsigned long len) {
	TW_CopyBytes(dst, src, len);
	return dst;
}

void *memset(void *dst, int byte, unsigned long len) {
	TW_FillBytes(dst, byte, len);
	return dst;
}
