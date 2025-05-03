#include <twilight_ppc.h>

typedef unsigned int u32;

int main() {
	TwVideo video_params;
	TW_InitVideo(&video_params);

	TwTerminal term_params = {
		.column = 10,
		.row = 6,
		.fore = 0xff80ff80,
		.back = 0x00800080,
	};
	TW_WriteTerminalAscii(&term_params, &video_params, "hello", 5);

	return 0;
}
