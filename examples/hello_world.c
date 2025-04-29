#include <twilight.h>

typedef unsigned int u32;

int main() {
	TW_DataVideoInit video_params;
	TW_VideoInit(&video_params);

	TW_DataTerminal term_params = {
		.col = 10,
		.row = 6,
		.fore = 0xff80ff80,
		.back = 0x00800080,
	};
	TW_TerminalWriteAscii(&term_params, "hello", 5);

	return 0;
}
