#include <twilight_ppc.h>

#define NULL (void*)0

typedef unsigned int u32;

int main() {
	TwVideoParams video = {};
	TW_InitVideo(&video);

	TwFile *sd = TW_OpenSdCard();
	TwFilesystem sdFs = TW_MountFirstFilesystem(sd, "/sd");
	if (sdFs.partition.sizeBytes == 0) {
		TW_Printf("\n\nCould not open SD card, exiting...");
		return 1;
	}

	TwFile *imageFile = TW_OpenFile(TW_MODE_RDWR, "/sd/paint.pb");
	if (!imageFile) {
		imageFile = TW_CreateFile(TW_MODE_RDWR, 0, "/sd/paint.pb");
		if (!imageFile) {
			TW_Printf("\n\nFailed to open or create /sd/paint.bmp on SD card");
			return 2;
		}
	}
	else {
		loadImage(video, imageFile);
	}

	unsigned portMask = 1;
	TW_SetupSerialDevices(portMask);

	float cursorX = 100;
	float cursorY = 100;
	float prevCursorX = 0;
	float prevCursorY = 0;
	int wasPainting = 0;
	u32 color = 0xff80ff80;

	while (1) {
		TwSerialInput input = TW_GetSerialInputs(0);

		if (input.gamecube.buttons & 0x1000) {
			break;
		}

		int xStick = (int)input.gamecube.analogX - 128;
		int yStick = (int)input.gamecube.analogY - 128;
		if (xStick <= -32 || xStick >= 32)
			cursorX += (float)xStick / 24.0f;
		if (yStick <= -32 || yStick >= 32)
			cursorY += (float)yStick / 24.0f;

		if (input.gamecube.buttons & 0x0100) {
			if (wasPainting)
				drawPath(video, color, prevCursorX, prevCursorY, cursorX, cursorY);
			else
				drawCircle(video, color, cursorX, cursorY);
		}

		prevCursorX = cursorX;
		prevCursorY = cursorY;
		wasPainting = (input.gamecube.buttons & 0x0100) != 0;
	}

	saveImage(video, imageFile);
	sd->close(sd);

	return 0;
}
