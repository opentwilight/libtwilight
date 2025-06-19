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

int decompressPackbitsRgbToYuyv(unsigned *outData, int outOffset, int outSize, int screenWidth, int vertBorderW, char *input, int length) {
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
			if (outOffset % screenWidth >= screenWidth - vertBorderW)
				outOffset += vertBorderW * 2;
			outData[outOffset++] = TW_RgbaToYuyv((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff, 255);
		}
	}

	return outOffset;
}

int compressPackbitsYuyvToRgb(TwFlexArray *output, int screenWidth, int vertBorderW, unsigned *input, int inOffset, int inSize) {
	char last128[128];
	int posLast128 = 0;
    int diffLength = 0;
    int runLength = 0;
    char run[2];
    char prev = 0;

	for (int i = inOffset; i < inSize; i++) {
		if (i % screenWidth >= screenWidth - vertBorderW) {
			i += vertBorderW * 2;
			if (i >= inSize)
				break;
		}

		unsigned yuyv = input[i];
		unsigned rgb = TW_YuvToRgb((yuyv >> 24) & 0xff, (yuyv >> 16) & 0xff, yuyv & 0xff);

		for (int j = 0; j < 3; j++) {
			char b = (char)((rgb >> (8*(2-j))) & 0xff);
            if (diffLength >= 0x7e || b == prev) {
                if (diffLength >= 2) {
                	char hdr = (char)(diffLength - 2);
                	TW_AppendFlexArray(output, &hdr, 1);
                	int pos = (posLast128 - diffLength + 128) & 0x7f;
                	int wrapLen = (diffLength - 1) - (128 - pos);
                	if (wrapLen <= 0) {
                		TW_AppendFlexArray(output, &last128[pos], diffLength - 1);
                	}
                	else {
            			TW_AppendFlexArray(output, &last128[pos], 128 - pos);
            			TW_AppendFlexArray(output, &last128[0], wrapLen);
        			}
                    runLength = 1;
                }
                if (b == prev)
                    diffLength = 0;
            }
            if ((runLength & 0x7f) == 0x7f || (b != prev && i > 0)) {
                if (runLength >= 2) {
                	run[0] = (char)(1 - (runLength & 0x7f));
                	run[1] = prev;
                	TW_AppendFlexArray(output, &run[0], 2);
                    diffLength = 0;
                }
                if (b != prev && i > 0)
                    runLength = 0;
                else
                    runLength++;
            }
            last128[posLast128++] = b;
            posLast128 &= 0x7f;
            runLength++;
            diffLength++;
            prev = b;
		}
	}
    if (runLength > 1) {
    	run[0] = (char)(1 - (runLength & 0x7f));
    	run[1] = prev;
    	TW_AppendFlexArray(output, &run[0], 2);
    }
    else if (diffLength > 0) {
    	char hdr = (char)(diffLength - 2);
    	TW_AppendFlexArray(output, &hdr, 1);
    	int pos = (posLast128 - diffLength + 128) & 0x7f;
    	int wrapLen = (diffLength - 1) - (128 - pos);
    	if (wrapLen <= 0) {
    		TW_AppendFlexArray(output, &last128[pos], diffLength - 1);
    	}
    	else {
			TW_AppendFlexArray(output, &last128[pos], 128 - pos);
			TW_AppendFlexArray(output, &last128[0], wrapLen);
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
		HUD_SIDE,
		data,
		fileSize
	);

	TW_Free(NULL, data);
	return 0;
}

int saveImage(TwVideo *video, TwFile *imageFile) {
	long long seekRes = imageFile->seek(imageFile, 0, TW_SEEK_SET);
	if (seekRes == -1LL) {
		TW_Printf("Failed to seek to beginning of image file for output");
		return -1;
	}

	MyImageFormat header = {
		.magic = 0x696d5042,
		.width = video->width - 2 * HUD_SIDE,
		.height = (video->height - HUD_BOTTOM - HUD_TOP) * video->width,
	};

	int res = imageFile->write(imageFile, &header, sizeof(MyImageFormat));
	if (res != sizeof(MyImageFormat)) {
		TW_Printf("Failed to write image header");
		return -2;
	}

	TwFlexArray output = TW_MakeFlexArray(NULL, video->width);
	compressPackbitsYuyvToRgb(
		&output,
		video->width,
		HUD_SIDE,
		video->xfb,
		HUD_TOP * video->width,
		header.height
	);

	res = imageFile->write(imageFile, output.data, output.size);
	TW_FreeFlexArray(&output);

	if (res != output.size) {
		TW_Printf("Failed to write the full image contents (%d/%d)", res, output.size);
		return -3;
	}

	return 0;
}

void drawCircle(TwVideo *video, unsigned color, int radius, int cursorX, int cursorY) {
	unsigned *xfb = video->xfb;
	int width = video->width;
	int height = video->height;
	for (int i = -radius; i < radius; i++) {
		for (int j = -radius; j < radius; j++) {
			int x = cursorX + j;
			int y = cursorY + i;
			if (
				x >= HUD_SIDE &&
				y >= HUD_TOP &&
				x < width - HUD_SIDE &&
				y < height - HUD_BOTTOM &&
				i*i + j*j <= radius*radius
			) {
				xfb[x + width * y] = color;
			}
		}
	}
}

void drawPath(TwVideo *video, unsigned color, int radius, int prevCursorX, int prevCursorY, int cursorX, int cursorY) {
	int dx = cursorX - prevCursorX;
	int dy = cursorY - prevCursorY;
	int lenSq = dx*dx + dy*dy;
	if (lenSq == 0) {
		drawCircle(video, color, radius, cursorX, cursorY);
		return;
	}

	double lenFloat = 1.0 / PPC_INV_SQRT((double)lenSq);
	int length = (int)(lenFloat + 0.5);
	if (length <= 0) {
		drawCircle(video, color, radius, cursorX, cursorY);
		return;
	}

	unsigned *xfb = video->xfb;
	int width = video->width;
	int height = video->height;

	double rx = (double)dx / (double)length;
	double ry = (double)dy / (double)length;
	double xMin = HUD_SIDE;
	double xMax = width - HUD_SIDE;
	double yMin = HUD_TOP;
	double yMax = height - HUD_BOTTOM;

	for (int i = -radius; i < radius; i++) {
		for (int j = 0; j < length; j++) {
			double xIn = cursorX + j;
			double yIn = cursorY + i;
			double xOut = rx * xIn - ry * yIn;
			double yOut = ry * xIn + rx * yIn;
			if (xOut >= xMin && xOut < xMax && yOut >= yMin && yOut < yMax)
				xfb[(int)xOut + width * (int)yOut] = color;
		}
	}

	drawCircle(video, color, radius, cursorX, cursorY);
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

	int radius = 5;
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
				drawPath(&video, color, radius, prevCursorX, prevCursorY, cursorX, cursorY);
			else
				drawCircle(&video, color, radius, cursorX, cursorY);
		}

		prevCursorX = cursorX;
		prevCursorY = cursorY;
		wasPainting = (input.gamecube.buttons & 0x0100) != 0;
	}

	saveImage(&video, imageFile);
	sd->close(sd);

	return 0;
}
