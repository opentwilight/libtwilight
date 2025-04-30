#include "twilight.h"
#include "font.h"

static int _initialized = 0;

static void init_video(TW_DataVideoInit *params) {
	params->width = 320;
	params->height = 480;
	params->xfb = (unsigned int*)TW_Allocate(params->width * params->height, sizeof(unsigned int));
}

void TW_VideoInit(TW_DataVideoInit *params) {
	TW_TwilightInit();
	if (!_initialized) {
		init_video(params);
		_initialized = 1;
	}
}

void TW_TerminalWriteAscii(TW_DataTerminal *params, const char *chars, int len) {
	
}
