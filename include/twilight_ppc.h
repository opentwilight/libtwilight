#pragma once

#include "twilight_common.h"

#define POKE_SPR(nStr, valueVar) __asm("mtspr " nStr ", %0" : : "b"(valueVar))

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
void TW_DrawAsciiSpan(TwVideo *video, TwTermFont *font, unsigned back, unsigned fore, float x, float y, const char *str, int count);
int TW_PrintTerminal(TwTerminal *term, TwVideo *video, const char *chars, int len);

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

// TODO: PPC-specific spinlocks, parking, atomics, threads
// threading_ppc.c
// Timer interrupt configurable via DEC register (SPR 22)

// misc.S
extern void *TW_FlushMemory(void *ptr, int size);
extern void *TW_FillWordsAndFlush(void *ptr, unsigned value, int words);
extern unsigned TW_CountLeadingZeros(unsigned value);
extern int TW_CountBits(unsigned value);
extern int TW_HasOneBitSet(unsigned value);
