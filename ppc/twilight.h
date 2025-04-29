#pragma once

typedef struct {
	u32 *xfb;
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
