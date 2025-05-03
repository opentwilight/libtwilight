#pragma once

#include "../common/twilight_common.h"

#define TW_MEM1_START 0x80000000
#define TW_MEM1_END   0x81800000
#define TW_MEM2_START 0x90000000
#define TW_MEM2_END   0x94000000

#define TW_IOS_MEM_START 0x93300000

#define VI_REG_BASE 0xCC002000

#define TW_PPC_COUNT_LEADING_ZEROS(output, input) __asm("cntlzd %0,%1" : "w"(output) : "r"(input))

typedef struct {
	unsigned *xfb;
	int width;
	int height;
	int format;
	int is_progressive;
} TwVideo;

typedef struct {
	int column;
	int row;
	unsigned fore;
	unsigned back;
	int disable_scroll;
	int disable_wrap;
} TwTerminal;

// twilight.c
void TW_InitTwilight(void);
unsigned TW_GetFramebufferAddress(int *outSize);
void *TW_AllocateGlobal(int count, int elemSize);
void TW_FreeGlobal(void *ptr);

// video.c
void TW_InitVideo(TwVideo *params);
void TW_WriteTerminalAscii(TwTerminal *params, TwVideo *video, const char *chars, int len);

// TODO: PPC-specific spinlocks, parking, atomics, threads
