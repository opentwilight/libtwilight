#include <twilight_ppc.h>

#define NULL (void*)0

#define COLOR_WHITE 0xff80ff80
#define COLOR_BLACK 0x00800080

typedef unsigned int u32;

int main() {
	TwVideo videoParams;
	TW_InitVideo(&videoParams);

	unsigned portMask = 1;
	TW_SetupSerialDevices(portMask);

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

		TW_ClearVideoScreen(&videoParams, COLOR_BLACK);
		TW_DrawAsciiSpan(&videoParams, NULL, COLOR_BLACK, COLOR_WHITE, x, y, "hello", 5);
		TW_AwaitVideoVBlank(&videoParams);

		counter++;
	}

	return 0;
}
