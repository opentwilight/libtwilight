#pragma once

// devices are laid out as u8 btaddr[6]; char name[64];
#define MAX_PAD_DEVICES 16
#define PAD_DEVICE_SIZE (6 + 64)

typedef struct {
	unsigned int irSensitivity;
	unsigned int counterBias;
	unsigned int wiiConnect24;
	unsigned short shutdownModeAndIdleLedMode;
	unsigned char isProgressiveScan;
	unsigned char isEuRgb60;
	unsigned char sensorBarPosition;
	unsigned char padSpeakerVolume;
	unsigned char padMotorMode;
	unsigned char soundMode;
	unsigned char language;
	unsigned char screenSaverMode;
	unsigned char eula;
	unsigned char aspectRatio;
	char displayOffsetH;
	char region[3];
	char area[4];
	char video[5];
	char nickname[22]; // only 10 bytes used
	char parentalPasswordAndAnswer[74]; // only 40 bytes used, bytes 4-7 are the password, bytes 8-39 are the answer
	unsigned char padDeviceCountAndDevices[1 + MAX_PAD_DEVICES * (PAD_DEVICE_SIZE)];
} TW_DataSysconf;

int TW_ReadSysconf(TW_DataSysconf *config);
