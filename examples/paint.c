#include <twilight_ppc.h>

#define HUD_SIDE    16
#define HUD_TOP     32
#define HUD_BOTTOM  32

#define NULL (void*)0

typedef unsigned int u32;

typedef struct {
	unsigned magic;
	unsigned flags;
	short width;
	short height;
} MyImageFormat;

int decompressPackbitsRgbToYuyv(unsigned *outData, int outOffset, int outSize, int screenWidth, char *input, int length) {
	int isRepeat = 0;
	int left = 0;
	unsigned rgb = 0;
	int toWrite = 0;

	for (int i = 0; i < length && outOffset < outSize; i++) {
		if (left == 0) {
			int mode = input[i];
			isRepeat = mode < 0;
			left = mode * (1 - (isRepeat * 2)) + 1;
			continue;
		}

		unsigned b = ((unsigned)input[i] & 0xff);
		if (isRepeat) {
			int j;
			for (j = 0; j < left && toWrite < 3; j++) {
				rgb = (rgb << 8) | b;
				toWrite++;
			}
			left -= j;
		}
		else {
			rgb = (rgb << 8) | b;
			toWrite++;
			left--;
		}

		if (toWrite >= 3) {
			if (outOffset % screenWidth >= screenWidth - HUD_SIDE)
				outOffset += HUD_SIDE * 2;
			outData[outOffset++] = TW_RgbaToYuyv((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff, 255);
		}
	}

	return outOffset;
}

int compressPackbitsYuyvToRgb(TwFlexArray *output, int screenWidth, unsigned *input, int length) {
	for (int i = 0; i < length; i++) {
		unsigned yuyv = input[i];
		unsigned rgb = TW_YuvToRgb((yuyv >> 24) & 0xff, (yuyv >> 16) & 0xff, yuyv & 0xff);

		for (int j = 0; j < 3; j++) {
			
		}
	}
	return 0;
}

int loadImage(TwVideo *video, TwFile *imageFile) {
	TwFileProperties fileProps = imageFile->getProperties(imageFile);
	if (fileProps.totalSize <= 0 || fileProps.totalSize >= 0x80000000LL) {
		TW_Printf("Image file could not be read (size: %lld)\n", fileProps.totalSize);
		return -1;
	}
	int fileSize = (int)fileProps.totalSize;

	MyImageFormat header;
	if (imageFile->read(imageFile, &header, sizeof(MyImageFormat)) != sizeof(MyImageFormat)) {
		TW_Printf("Failed to read header from image file");
		return -2;
	}

	int expectedW = video->width - 2 * HUD_SIDE;
	int expectedH = (video->height - HUD_BOTTOM - HUD_TOP) * video->width;
	if (header.magic != 0x696d5042 || header.width != expectedW || header.height != expectedH) {
		TW_Printf("Invalid header: file was not a imPB image, or the size wasn't %d x %d", expectedW, expectedH);
		return -3;
	}

	char *data = TW_Allocate(NULL, NULL, fileSize, 1);
	if (!data) {
		TW_Printf("Failed to allocate %lld bytes of image data", fileSize);
		return -4;
	}

	int res = imageFile->read(imageFile, data, fileSize);
	if (res <= 0) {
		TW_Printf("Failed to read image data past the header");
		TW_Free(NULL, data);
		return -5;
	}

	decompressPackbitsRgbToYuyv(
		video->xfb,
		HUD_TOP * video->width,
		(video->height - HUD_BOTTOM - HUD_TOP) * video->width,
		video->width,
		data,
		fileSize
	);

	TW_Free(NULL, data);
	return 0;
}

void saveImage(TwVideo *video, TwFile *imageFile) {

}

void drawPath(TwVideo *video, unsigned color, int prevCursorX, int prevCursorY, int cursorX, int cursorY) {

}

void drawCircle(TwVideo *video, unsigned color, int cursorX, int cursorY) {

}

int main() {
	TwVideo video = {};
	TW_InitVideo(&video);
	TW_Printf("\n\n");

	TwFile *sd = TW_OpenSdCard();
	TwFilesystem sdFs = TW_MountFirstFilesystem(sd, "/sd");
	if (sdFs.partition.sizeBytes == 0) {
		TW_Printf("Could not open SD card, exiting...");
		return 1;
	}

	TwFile *imageFile = TW_OpenFile(TW_MODE_RDWR, "/sd/paint.pb");
	if (!imageFile) {
		imageFile = TW_CreateFile(TW_MODE_RDWR, 0, "/sd/paint.pb");
		if (!imageFile) {
			TW_Printf("Failed to open or create /sd/paint.pb on SD card");
			return 2;
		}
	}
	else {
		loadImage(&video, imageFile);
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

		if (input.gamecube.buttons & 0x1000)
			break;

		int xStick = (int)input.gamecube.analogX - 128;
		int yStick = (int)input.gamecube.analogY - 128;
		if (xStick <= -32 || xStick >= 32)
			cursorX += (float)xStick / 24.0f;
		if (yStick <= -32 || yStick >= 32)
			cursorY += (float)yStick / 24.0f;

		if (input.gamecube.buttons & 0x0100) {
			if (wasPainting)
				drawPath(&video, color, prevCursorX, prevCursorY, cursorX, cursorY);
			else
				drawCircle(&video, color, cursorX, cursorY);
		}

		prevCursorX = cursorX;
		prevCursorY = cursorY;
		wasPainting = (input.gamecube.buttons & 0x0100) != 0;
	}

	saveImage(&video, imageFile);
	sd->close(sd);

	return 0;
}
