#pragma once

#include "../common/twilight_common.h"

#define POKE_SPR(nStr, valueVar) __asm("mtspr " nStr ", %0" : : "b"(valueVar))

#if TW_WII
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

#if TW_WII
#define TW_SERIAL_REG_BASE  0xCD006400
#else
#define TW_SERIAL_REG_BASE  0xCC006400
#endif

#define TW_VIDEO_NTSC  0
#define TW_VIDEO_PAL50 1
#define TW_VIDEO_MPAL  2

#define TW_PPC_COUNT_LEADING_ZEROS(output, input) __asm("cntlzd %0,%1" : "w"(output) : "r"(input))

typedef struct {
	unsigned *xfb;
	int width;
	int height;
	int format;
	int is_progressive;
} TwVideo;

typedef struct {
	int column;
	int row;
	unsigned fore;
	unsigned back;
	int disable_scroll;
	int disable_wrap;
} TwTerminal;

// twilight.c
void TW_InitTwilight(void);
unsigned TW_GetFramebufferAddress(int *outSize);
void *TW_AllocateGlobal(int count, int elemSize);
void TW_FreeGlobal(void *ptr);

// video.c
void TW_InitVideo(TwVideo *params);
void TW_ClearVideoScreen(TwVideo *params, unsigned color);
void TW_WriteTerminalAscii(TwTerminal *params, TwVideo *video, const char *chars, int len);

// serial.c
void TW_SetSerialPollInterval(unsigned line, unsigned count);

// interrupts.c
void TW_SetTimerInterrupt(void (*handler)(), unsigned quadCycles);
void TW_SetCpuInterruptHandler(int interruptType, void (*handler)());
void TW_SetExternalInterruptHandler(int interruptType, void (*handler)());

// audio.c
void TW_SetAudioInterrupts(void);

// dsp.c
void TW_SetDspInterrupts(void);

// exi.c
void TW_SetExiInterrupts(void);

// TODO: PPC-specific spinlocks, parking, atomics, threads
// threading_ppc.c
// Timer interrupt configurable via DEC register (SPR 22)

// misc.S
extern unsigned TW_EnableInterrupts(void);
extern unsigned TW_DisableInterrupts(void);
extern void *TW_FlushMemory(void *ptr, int size);
