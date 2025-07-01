#pragma once

#include "twilight.h"

#define PPC_SYNC() __asm("sync")
#define PPC_ISYNC() __asm("isync")
#define PPC_DCBF(ptr) __asm("dcbf 0, %0" : : "b"(ptr))
#define PPC_DCBI(ptr) __asm("dcbi 0, %0" : : "b"(ptr) : "memory")
#define PPC_POKE_SPR(nStr, valueVar) __asm("mtspr " nStr ", %0" : : "b"(valueVar))
#define PPC_INV_SQRT(outValue, inValue) __asm("frsqrte %0, %1" : "=f"(outValue) : "f"(inValue) : )

#define GET_PHYSICAL_POINTER(x) (void*)((unsigned)(x) & 0x3fffFFFF)

#define TW_PPC_MAX_THREADS 128

#ifdef TW_WII
#define TW_INTERRUPT_BIT_STARLET_TIMER (16 + 0)
#define TW_INTERRUPT_BIT_NAND          (16 + 1)
#define TW_INTERRUPT_BIT_AES           (16 + 2)
#define TW_INTERRUPT_BIT_SHA1          (16 + 3)
#define TW_INTERRUPT_BIT_USB_E         (16 + 4)
#define TW_INTERRUPT_BIT_USB_O0        (16 + 5)
#define TW_INTERRUPT_BIT_USB_O1        (16 + 6)
#define TW_INTERRUPT_BIT_SD            (16 + 7)
#define TW_INTERRUPT_BIT_WIFI          (16 + 8)
#define TW_INTERRUPT_BIT_GPIO_BROADWAY (16 + 10)
#define TW_INTERRUPT_BIT_GPIO_STARLET  (16 + 11)
#define TW_INTERRUPT_BIT_MIOS          (16 + 15)
#define TW_INTERRUPT_BIT_WRESET        (16 + 17)
#define TW_INTERRUPT_BIT_WDVD          (16 + 18)
#define TW_INTERRUPT_BIT_VWII          (16 + 19)
#define TW_INTERRUPT_BIT_IPC_BROADWAY  (16 + 30)
#define TW_INTERRUPT_BIT_IPC_STARLET   (16 + 31)

#define TW_INTERRUPT_BIT_ACR       14
#endif
#define TW_INTERRUPT_BIT_HSP       13
#define TW_INTERRUPT_BIT_DEBUG     12
#define TW_INTERRUPT_BIT_FIFO      11
#define TW_INTERRUPT_BIT_PE_FINISH 10
#define TW_INTERRUPT_BIT_PE_TOKEN   9
#define TW_INTERRUPT_BIT_VIDEO      8
#define TW_INTERRUPT_BIT_MEMORY     7
#define TW_INTERRUPT_BIT_DSP        6
#define TW_INTERRUPT_BIT_AUDIO      5
#define TW_INTERRUPT_BIT_EXI        4
#define TW_INTERRUPT_BIT_SERIAL     3
#define TW_INTERRUPT_BIT_DVD        2
#define TW_INTERRUPT_BIT_RESET      1
#define TW_INTERRUPT_BIT_ERROR      0

#define TW_INTERRUPT_TYPE_RESET           1
#define TW_INTERRUPT_TYPE_MACH_CHECK      2
#define TW_INTERRUPT_TYPE_DSI             3
#define TW_INTERRUPT_TYPE_ISI             4
#define TW_INTERRUPT_TYPE_EXTERNAL        5
#define TW_INTERRUPT_TYPE_ALIGNMENT       6
#define TW_INTERRUPT_TYPE_PROGRAM         7
#define TW_INTERRUPT_TYPE_FP_UNAVAILABLE  8
#define TW_INTERRUPT_TYPE_DECREMENTER     9
#define TW_INTERRUPT_TYPE_SYSCALL        12
#define TW_INTERRUPT_TYPE_TRACE          13
#define TW_INTERRUPT_TYPE_FP_ASSIST      14
#define TW_INTERRUPT_TYPE_INS_BREAKPOINT 15

#define TW_MEM1_START 0x80000000
#define TW_MEM1_END   0x81800000
#define TW_MEM2_START 0x90000000
#define TW_MEM2_END   0x94000000

#define TW_IOS_MEM_START 0x93300000

#define TW_VIDEO_REG_BASE      0xCC002000
#define TW_INTERRUPT_REG_BASE  0xCC003000

#ifdef TW_WII
#define TW_SERIAL_REG_BASE  0xCD006400
#define TW_IRQ_WII_REG_BASE 0xD0000030
#else
#define TW_SERIAL_REG_BASE  0xCC006400
#endif

#define TW_VIDEO_NTSC  0
#define TW_VIDEO_PAL50 1
#define TW_VIDEO_MPAL  2

#define TW_DISABLE_WRAP   1
#define TW_DISABLE_SCROLL 2

#define TW_MAX_ANSI_ESC 32

#define TW_PPC_COUNT_LEADING_ZEROS(output, input) __asm("cntlzd %0,%1" : "w"(output) : "r"(input))

#ifdef TW_WII

#define TW_IOS_CMD_OPEN   1
#define TW_IOS_CMD_CLOSE  2
#define TW_IOS_CMD_READ   3
#define TW_IOS_CMD_WRITE  4
#define TW_IOS_CMD_SEEK   5
#define TW_IOS_CMD_IOCTL  6
#define TW_IOS_CMD_IOCTLV 7

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
} TwSysconf;

#endif

typedef struct {
	unsigned gprs[32];
	double fprs[32];
	unsigned pc;
	unsigned lr;
	unsigned xer;
	unsigned ctr;
	unsigned cr;
	unsigned fpscr;
} TwPpcCpuContext;

typedef union {
	unsigned words[2];
	struct {
		// e = error, s = Start, y = Y Button, x = X Button, b = B Button, a = A Button
		// L = Left Trigger, R = Right Trigger, z = Z Button, udrl = D-Pad directions
		// -------------------
		// ee_s yxba _LRz udrl
		// 0123 4567 89ab cdef
		// -------------------
		unsigned short buttons; // see above
		unsigned char analogX; // 32 = max left, 128 = neutral, 224 = max right
		unsigned char analogY; // 32 = max up, 128 = neutral, 224 max down
		unsigned char cStickX; // 32 = max left, 128 = neutral, 224 = max right
		unsigned char cStickY; // 32 = max up, 128 = neutral, 224 max down
		unsigned char triggerL; // 32 = minimum, 224 maximum
		unsigned char triggerR; // 32 = minimum, 224 maximum
	} gamecube;
} TwSerialInput;

typedef struct {
	unsigned *xfb;
	int width;
	int height;
	int format;
	int isProgressive;
} TwVideo;

typedef struct {
	int column;
	int row;
	unsigned fore;
	unsigned back;
	unsigned flags;
	int ansiEscLen;
	char ansiEscBuf[TW_MAX_ANSI_ESC];
} TwTerminal;

typedef struct {
	const unsigned char *data;
	int width;
	int height;
	int bytesPerGlyph;
	int count;
} TwTermFont;

// twilight.c
void TW_InitTwilight(void);
unsigned TW_GetFramebufferAddress(int *outSize);

// video.c
void TW_InitVideo(TwVideo *params);
TwVideo *TW_GetDefaultVideo(void);
void TW_AwaitVideoVBlank(TwVideo *params);
void TW_ClearVideoScreen(TwVideo *params, unsigned color);
unsigned TW_RgbaToYuyv(int r, int g, int b, int a);
unsigned TW_YuvToRgb(int y, int u, int v);
void TW_DrawAsciiSpan(TwVideo *video, TwTermFont *font, unsigned back, unsigned fore, float x, float y, const char *str, int count);
int TW_PrintTerminal(TwTerminal *term, TwTermFont *font, TwVideo *video, const char *chars, int len);

// serial.c
void TW_SetSerialPollInterval(unsigned line, unsigned count);
void TW_SetupSerialDevices(unsigned portMask);
TwSerialInput TW_GetSerialInputs(int port);

// interrupts.c
void TW_SetTimerInterrupt(void (*handler)(), unsigned quadCycles);
void TW_SetCpuInterruptHandler(int interruptType, void (*handler)());
void TW_SetExternalInterruptHandler(int interruptType, void (*handler)());

// audio.c
void TW_InitAudioInterrupts(void);

// dsp.c
void TW_InitDspInterrupts(void);

// exi.c
void TW_InitExiInterrupts(void);

#ifdef TW_WII

// es.c
int TW_LaunchWiiTitle(unsigned long long titleId);

// ios.c
void TW_SetupIos(void);
void TW_InvokeMatchingIosCompletionHandler(unsigned *iob);
int TW_IoctlvRebootIos(int fd, unsigned method, int nInputs, int nOutputs, TwView *inputsAndOutputs);
TwFilesystem TW_MakeIosFilesystem(void);

// sd.c
TwFile *TW_OpenSdCard(void);

// sysconf.c
int TW_ReadSysconf(TwSysconf *config);

#endif

// misc.S
extern void *TW_GetTocPointer(void);
extern void *TW_GetInterruptStackStart(void);
extern void *TW_GetMainStackStart(void);
extern unsigned TW_EnableInterrupts(void);
extern unsigned TW_DisableInterrupts(void);
extern void TW_ZeroAndFlushBlock(unsigned *cacheBlockAlignedAddress);
extern void *TW_FlushMemory(void *ptr, int size);
extern void *TW_SyncAfterWrite(void *ptr, int size);
extern void *TW_SyncBeforeRead(void *ptr, int size);
extern void *TW_FillWordsAndFlush(void *ptr, unsigned value, int words);
extern unsigned *TW_GetStoredRegistersAddress(void);
