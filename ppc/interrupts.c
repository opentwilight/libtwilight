#include <twilight_ppc.h>

#define MAYBE_INVOKE_THEN_RETURN(value, offset, bit) \
	if ((value & (1 << (bit - offset))) != 0) { \
		if (__tw_external_interrupt_handler_table[bit]) \
			__tw_external_interrupt_handler_table[bit](); \
		return; \
	} \

extern void (*__tw_external_interrupt_handler_table[])(void);
extern void (*__tw_cpu_interrupt_handler_table[])(void);

// timer interrupt -- perfect for real multithreading
static void (*_tw_decrement_underflow_handler)(void);

static void _tw_dispatch_external_interrupt() {
	unsigned cause = PEEK_U32(TW_INTERRUPT_REG_BASE + 0) & 0x7fff;
	unsigned mask  = PEEK_U32(TW_INTERRUPT_REG_BASE + 4) & 0x7fff;
	unsigned active = cause & mask;
	if (!active)
		return;

	MAYBE_INVOKE_THEN_RETURN(active, 0, TW_INTERRUPT_BIT_ERROR)
	MAYBE_INVOKE_THEN_RETURN(active, 0, TW_INTERRUPT_BIT_DEBUG)
	MAYBE_INVOKE_THEN_RETURN(active, 0, TW_INTERRUPT_BIT_MEMORY)
	MAYBE_INVOKE_THEN_RETURN(active, 0, TW_INTERRUPT_BIT_RESET)
	MAYBE_INVOKE_THEN_RETURN(active, 0, TW_INTERRUPT_BIT_VIDEO)
	MAYBE_INVOKE_THEN_RETURN(active, 0, TW_INTERRUPT_BIT_PE_TOKEN)
	MAYBE_INVOKE_THEN_RETURN(active, 0, TW_INTERRUPT_BIT_PE_FINISH)
	MAYBE_INVOKE_THEN_RETURN(active, 0, TW_INTERRUPT_BIT_HSP)
	MAYBE_INVOKE_THEN_RETURN(active, 0, TW_INTERRUPT_BIT_DSP)
	MAYBE_INVOKE_THEN_RETURN(active, 0, TW_INTERRUPT_BIT_AUDIO)
	MAYBE_INVOKE_THEN_RETURN(active, 0, TW_INTERRUPT_BIT_EXI)
	MAYBE_INVOKE_THEN_RETURN(active, 0, TW_INTERRUPT_BIT_SERIAL)
	MAYBE_INVOKE_THEN_RETURN(active, 0, TW_INTERRUPT_BIT_DVD)
	MAYBE_INVOKE_THEN_RETURN(active, 0, TW_INTERRUPT_BIT_FIFO)
#ifdef TW_WII
	if ((active & (1 << TW_INTERRUPT_BIT_ACR)) != 0) {
		cause = PEEK_U32(TW_IRQ_WII_REG_BASE + 0x30);
		mask  = PEEK_U32(TW_IRQ_WII_REG_BASE + 0x34);
		active = cause & mask;
		if (!active)
			return;

		MAYBE_INVOKE_THEN_RETURN(active, 16, TW_INTERRUPT_BIT_STARLET_TIMER)
		MAYBE_INVOKE_THEN_RETURN(active, 16, TW_INTERRUPT_BIT_NAND)
		MAYBE_INVOKE_THEN_RETURN(active, 16, TW_INTERRUPT_BIT_AES)
		MAYBE_INVOKE_THEN_RETURN(active, 16, TW_INTERRUPT_BIT_SHA1)
		MAYBE_INVOKE_THEN_RETURN(active, 16, TW_INTERRUPT_BIT_USB_E)
		MAYBE_INVOKE_THEN_RETURN(active, 16, TW_INTERRUPT_BIT_USB_O0)
		MAYBE_INVOKE_THEN_RETURN(active, 16, TW_INTERRUPT_BIT_USB_O1)
		MAYBE_INVOKE_THEN_RETURN(active, 16, TW_INTERRUPT_BIT_SD)
		MAYBE_INVOKE_THEN_RETURN(active, 16, TW_INTERRUPT_BIT_WIFI)
		MAYBE_INVOKE_THEN_RETURN(active, 16, TW_INTERRUPT_BIT_GPIO_BROADWAY)
		MAYBE_INVOKE_THEN_RETURN(active, 16, TW_INTERRUPT_BIT_GPIO_STARLET)
		MAYBE_INVOKE_THEN_RETURN(active, 16, TW_INTERRUPT_BIT_MIOS)
		MAYBE_INVOKE_THEN_RETURN(active, 16, TW_INTERRUPT_BIT_WRESET)
		MAYBE_INVOKE_THEN_RETURN(active, 16, TW_INTERRUPT_BIT_WDVD)
		MAYBE_INVOKE_THEN_RETURN(active, 16, TW_INTERRUPT_BIT_VWII)
		MAYBE_INVOKE_THEN_RETURN(active, 16, TW_INTERRUPT_BIT_IPC_BROADWAY)
		MAYBE_INVOKE_THEN_RETURN(active, 16, TW_INTERRUPT_BIT_IPC_STARLET)
	}
#endif
}

void _tw_dispatch_interrupt(unsigned irqIndex) {
	if (irqIndex == 5) {
		_tw_dispatch_external_interrupt();
	}
	else if (irqIndex == 8) {
		if (_tw_decrement_underflow_handler)
			_tw_decrement_underflow_handler();
	}
	else {
		if (__tw_cpu_interrupt_handler_table[irqIndex])
			__tw_cpu_interrupt_handler_table[irqIndex]();
	}
}

void TW_SetTimerInterrupt(void (*handler)(), unsigned quadCycles) {
	_tw_decrement_underflow_handler = handler;
	POKE_SPR("22", quadCycles);
}

void TW_SetCpuInterruptHandler(int interruptType, void (*handler)()) {
	__tw_cpu_interrupt_handler_table[interruptType] = handler;
}

void TW_SetExternalInterruptHandler(int interruptType, void (*handler)()) {
	switch (interruptType) {
		case TW_INTERRUPT_BIT_EXI:
			__tw_external_interrupt_handler_table[TW_INTERRUPT_BIT_EXI] = handler;
			TW_InitExiInterrupts();
			POKE_U32(TW_INTERRUPT_REG_BASE + 4, (1 << TW_INTERRUPT_BIT_EXI) | PEEK_U32(TW_INTERRUPT_REG_BASE + 4));
			break;
		case TW_INTERRUPT_BIT_AUDIO:
			__tw_external_interrupt_handler_table[TW_INTERRUPT_BIT_AUDIO] = handler;
			TW_InitAudioInterrupts();
			POKE_U32(TW_INTERRUPT_REG_BASE + 4, (1 << TW_INTERRUPT_BIT_AUDIO) | PEEK_U32(TW_INTERRUPT_REG_BASE + 4));
			break;
		case TW_INTERRUPT_BIT_DSP:
			__tw_external_interrupt_handler_table[TW_INTERRUPT_BIT_DSP] = handler;
			TW_InitDspInterrupts();
			POKE_U32(TW_INTERRUPT_REG_BASE + 4, (1 << TW_INTERRUPT_BIT_DSP) | PEEK_U32(TW_INTERRUPT_REG_BASE + 4));
			break;
		case TW_INTERRUPT_BIT_MEMORY:
		case TW_INTERRUPT_BIT_ERROR:
		case TW_INTERRUPT_BIT_RESET:
		case TW_INTERRUPT_BIT_DVD:
		case TW_INTERRUPT_BIT_SERIAL:
		case TW_INTERRUPT_BIT_VIDEO:
		case TW_INTERRUPT_BIT_PE_TOKEN:
		case TW_INTERRUPT_BIT_PE_FINISH:
		case TW_INTERRUPT_BIT_FIFO:
		case TW_INTERRUPT_BIT_DEBUG:
		case TW_INTERRUPT_BIT_HSP:
			__tw_external_interrupt_handler_table[interruptType] = handler;
			POKE_U32(TW_INTERRUPT_REG_BASE + 4, (1 << interruptType) | PEEK_U32(TW_INTERRUPT_REG_BASE + 4));
			break;
#ifdef TW_WII
		case TW_INTERRUPT_BIT_STARLET_TIMER:
		case TW_INTERRUPT_BIT_NAND:
		case TW_INTERRUPT_BIT_AES:
		case TW_INTERRUPT_BIT_SHA1:
		case TW_INTERRUPT_BIT_USB_E:
		case TW_INTERRUPT_BIT_USB_O0:
		case TW_INTERRUPT_BIT_USB_O1:
		case TW_INTERRUPT_BIT_SD:
		case TW_INTERRUPT_BIT_WIFI:
		case TW_INTERRUPT_BIT_GPIO_BROADWAY:
		case TW_INTERRUPT_BIT_GPIO_STARLET:
		case TW_INTERRUPT_BIT_MIOS:
		case TW_INTERRUPT_BIT_WRESET:
		case TW_INTERRUPT_BIT_WDVD:
		case TW_INTERRUPT_BIT_VWII:
		case TW_INTERRUPT_BIT_IPC_BROADWAY:
		case TW_INTERRUPT_BIT_IPC_STARLET:
			__tw_external_interrupt_handler_table[interruptType] = handler;
			POKE_U32(TW_IRQ_WII_REG_BASE + 0x34, (1 << (16 - interruptType)) | PEEK_U32(TW_IRQ_WII_REG_BASE + 0x34));
			break;
#endif
	}
}
