#include <twilight_ppc.h>

typedef unsigned int u32;

int main() {
	TwVideo video_params;
	TW_InitVideo(&video_params);

	TwTerminal term_params = {
		.column = 10,
		.row = 3,
		.fore = 0xff80ff80,
		.back = 0x00800080,
	};
	term_params.disable_scroll = 1;

	u32 counter = 0;
	while (1) {
		if ((counter % 30) == 0) {
			term_params.row = 3 + (counter & 7);
			TW_ClearVideoScreen(&video_params, term_params.back);
			TW_WriteTerminalAscii(&term_params, &video_params, "hello", 5);
		}
		counter++;
	}

	return 0;
}
