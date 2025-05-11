#include <twilight_ppc.h>

#define MAYBE_INVOKE_THEN_RETURN(value, bit) \
	if ((value & (1 << bit)) != 0) { \
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
	unsigned mask = PEEK_U32(TW_INTERRUPT_REG_BASE + 4) & 0x7fff;
	unsigned active = cause & mask;
	if (!active)
		return;

	MAYBE_INVOKE_THEN_RETURN(active, TW_INTERRUPT_BIT_ERROR)
	MAYBE_INVOKE_THEN_RETURN(active, TW_INTERRUPT_BIT_DEBUG)
	MAYBE_INVOKE_THEN_RETURN(active, TW_INTERRUPT_BIT_MEMORY)
	MAYBE_INVOKE_THEN_RETURN(active, TW_INTERRUPT_BIT_RESET)
	MAYBE_INVOKE_THEN_RETURN(active, TW_INTERRUPT_BIT_VIDEO)
	MAYBE_INVOKE_THEN_RETURN(active, TW_INTERRUPT_BIT_PE_TOKEN)
	MAYBE_INVOKE_THEN_RETURN(active, TW_INTERRUPT_BIT_PE_FINISH)
	MAYBE_INVOKE_THEN_RETURN(active, TW_INTERRUPT_BIT_HSP)
	MAYBE_INVOKE_THEN_RETURN(active, TW_INTERRUPT_BIT_DSP)
	MAYBE_INVOKE_THEN_RETURN(active, TW_INTERRUPT_BIT_AUDIO)
	MAYBE_INVOKE_THEN_RETURN(active, TW_INTERRUPT_BIT_EXI)
	MAYBE_INVOKE_THEN_RETURN(active, TW_INTERRUPT_BIT_SERIAL)
	MAYBE_INVOKE_THEN_RETURN(active, TW_INTERRUPT_BIT_DVD)
	MAYBE_INVOKE_THEN_RETURN(active, TW_INTERRUPT_BIT_FIFO)
#if TW_WII
	MAYBE_INVOKE_THEN_RETURN(active, TW_INTERRUPT_BIT_ACR)
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
			TW_SetExiInterrupts();
			POKE_U32(TW_INTERRUPT_REG_BASE + 4, (1 << TW_INTERRUPT_BIT_EXI) | PEEK_U32(TW_INTERRUPT_REG_BASE + 4));
			break;
		case TW_INTERRUPT_BIT_AUDIO:
			__tw_external_interrupt_handler_table[TW_INTERRUPT_BIT_AUDIO] = handler;
			TW_SetAudioInterrupts();
			POKE_U32(TW_INTERRUPT_REG_BASE + 4, (1 << TW_INTERRUPT_BIT_AUDIO) | PEEK_U32(TW_INTERRUPT_REG_BASE + 4));
			break;
		case TW_INTERRUPT_BIT_DSP:
			__tw_external_interrupt_handler_table[TW_INTERRUPT_BIT_DSP] = handler;
			TW_SetDspInterrupts();
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
#if TW_WII
		case TW_INTERRUPT_BIT_ACR:
#endif
			__tw_external_interrupt_handler_table[interruptType] = handler;
			POKE_U32(TW_INTERRUPT_REG_BASE + 4, (1 << interruptType) | PEEK_U32(TW_INTERRUPT_REG_BASE + 4));
			break;
	}
}
