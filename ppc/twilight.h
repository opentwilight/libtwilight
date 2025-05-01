#pragma once

#define MEM1_START 0x80000000
#define MEM1_END   0x81800000
#define MEM2_START 0x90000000
#define MEM2_END   0x94000000

#define IOS_MEM_START  0x93300000

typedef struct {
	unsigned int *xfb;
	int width;
	int height;
} TW_DataVideoInit;

typedef struct {
	int column;
	int row;
	unsigned int fore;
	unsigned int back;
} TW_DataTerminal;

void TW_TwilightInit(void);
void *TW_Allocate(int count, int elem_size);
void TW_Free(void *ptr);

void TW_VideoInit(TW_DataVideoInit *params);
void TW_TerminalWriteAscii(TW_DataTerminal *params, const char *chars, int len);
