#include <twilight_ppc.h>

typedef unsigned int u32;
#define NULL (void*)0

int main() {
	TwVideo video_params;
	TW_InitVideo(&video_params);

	unsigned portMask = 1;
	TW_SetupSerialDevices(portMask);

	TwTerminal term_params = {
		.column = 10,
		.row = 3,
		.fore = 0xff80ff80,
		.back = 0x00800080,
	};
	term_params.disable_scroll = 1;

	u32 counter = 0;
	int useCursor = 0;
	int cursorX = 100;
	int cursorY = 100;

	while (1) {
		int x = 0;
		int y = 0;

		TwSerialInput input = TW_GetSerialInputs(0);

		unsigned yxba = (input.gamecube.buttons & 0x0f00) >> 8;
		if (yxba != 0)  {
			if ((yxba & 8) != 0)
				cursorY--;
			if ((yxba & 4) != 0)
				cursorY++;
			if ((yxba & 2) != 0)
				cursorX--;
			if ((yxba & 1) != 0)
				cursorX++;
			useCursor = 1;
		}

		if (useCursor) {
			x = cursorX;
			y = cursorY;
		}
		else {
			x = -20 + (int)(counter % 360);
			y = -30 + (int)((counter >> 1) % 540);
		}

		TW_ClearVideoScreen(&video_params, 0x00800080);
		TW_DrawAsciiSpan(&video_params, NULL, 0x00800080, 0xff80ff80, x, y, "hello", 5);
		TW_AwaitVideoVBlank(&video_params);

		counter++;
	}

	return 0;
}
