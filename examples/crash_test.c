#include <twilight_ppc.h>

#define NULL (void*)0

#define COLOR_WHITE 0xff80ff80
#define COLOR_BLACK 0x00800080

typedef unsigned int u32;

int main() {
	TwVideo videoParams;
	TW_InitVideo(&videoParams);
	TW_ClearVideoScreen(&videoParams, COLOR_BLACK);

	unsigned portMask = 1;
	TW_SetupSerialDevices(portMask);

	TW_Printf("\x1b[6;10HCrash Test!");
	TW_Printf("\x1b[8;10HPress A or B to crash! \x7f");

	while (1) {
		TwSerialInput input = TW_GetSerialInputs(0);

		if (input.gamecube.buttons & 0x0100) {
			// pressed A
			*(unsigned*)0xff000000 = 1;
		}
		if (input.gamecube.buttons & 0x0200) {
			// pressed B
			void (*nowhere)() = (void (*)())0xff000000;
			nowhere();
		}

		TW_AwaitVideoVBlank(&videoParams);
	}

	return 0;
}
