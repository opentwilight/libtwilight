#include <twilight_ppc.h>

typedef unsigned int u32;
#define NULL (void*)0

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
		int x = -20 + (int)(counter & 0x7f); // -20 + ((int)(counter >> 1) % 120);
		int y = -30 + (int)((counter >> 1) & 0xff); // 80 + ((int)(counter >> 3) % 60);
		TW_ClearVideoScreen(&video_params, 0x00800080);
		TW_DrawAsciiSpan(&video_params, NULL, 0x00800080, 0xff80ff80, x, y, "hello", 5);
		TW_AwaitVideoVBlank(&video_params);
		counter++;
	}

	return 0;
}
