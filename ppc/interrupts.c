#include <twilight_ppc.h>

extern void (*__tw_external_interrupt_handler_table[])(void);

// timer interrupt -- perfect for real multithreading
static void (*_tw_decrement_underflow_handler)(void);

void _tw_interrupt_handler_impl(unsigned irqIndex) {
	int externalType = 0;

	switch (irqIndex) {
		case 8:
			if (_tw_decrement_underflow_handler)
				_tw_decrement_underflow_handler();
			break;
		default:
			if (__tw_external_interrupt_handler_table[externalType])
				__tw_external_interrupt_handler_table[externalType]();
			break;
	}
}

void TW_SetTimerInterrupt(void (*handler)(), unsigned quadCycles) {
	_tw_decrement_underflow_handler = handler;
	POKE_SPR("22", quadCycles);
}

void TW_SetInterruptHandler(int interruptType, void (*handler)()) {
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
		case TW_INTERRUPT_BIT_ACR:
			__tw_external_interrupt_handler_table[interruptType] = handler;
			POKE_U32(TW_INTERRUPT_REG_BASE + 4, (1 << interruptType) | PEEK_U32(TW_INTERRUPT_REG_BASE + 4));
			break;
	}
}
